/*
 * Taken from perf which in turn take it from GIT
 */

#include "kvm/util.h"

#include <kvm/kvm.h>
#include <linux/magic.h>	/* For HUGETLBFS_MAGIC */
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int debug_socket_fd = -1;

int setup_debug_socket(const char *socket_path)
{
  struct sockaddr_un addr;

  if ( (debug_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    printf("debug socket: socket error\n");
	return -1;
  }

  printf("debug socket: %s\n", socket_path);
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);

  if (connect(debug_socket_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    printf("debug socket: connect error\n");
	return -1;
  }

  return 0;
}


static void report(const char *prefix, const char *err, va_list params)
{
  char msg[1024];
  char xmsg[256+1024];
  vsnprintf(msg, sizeof(msg), err, params);
  snprintf(xmsg, sizeof(xmsg), " %s%s\n", prefix, msg);
  xmsg[256+1024-1] = (char)0;

  if (write(debug_socket_fd, xmsg, strlen(xmsg)) < 0)
	fprintf(stderr, " %s", xmsg);
}

static NORETURN void die_builtin(const char *err, va_list params)
{
	report(" Fatal: ", err, params);
	exit(128);
}

static void error_builtin(const char *err, va_list params)
{
	report(" Error: ", err, params);
}

static void warn_builtin(const char *warn, va_list params)
{
	report(" Warning: ", warn, params);
}

static void info_builtin(const char *info, va_list params)
{
	report(" Info: ", info, params);
}

void die(const char *err, ...)
{
	va_list params;

	va_start(params, err);
	die_builtin(err, params);
	va_end(params);
}

int pr_err(const char *err, ...)
{
	va_list params;

	va_start(params, err);
	error_builtin(err, params);
	va_end(params);
	return -1;
}

void pr_warning(const char *warn, ...)
{
	va_list params;

	va_start(params, warn);
	warn_builtin(warn, params);
	va_end(params);
}

void pr_info(const char *info, ...)
{
	va_list params;

	va_start(params, info);
	info_builtin(info, params);
	va_end(params);
}

void die_perror(const char *s)
{
	perror(s);
	exit(1);
}

void *mmap_hugetlbfs(struct kvm *kvm, const char *htlbfs_path, u64 size)
{
	char mpath[PATH_MAX];
	int fd;
	struct statfs sfs;
	void *addr;
	unsigned long blk_size;

	if (statfs(htlbfs_path, &sfs) < 0)
		die("Can't stat %s\n", htlbfs_path);

	if ((unsigned int)sfs.f_type != HUGETLBFS_MAGIC)
		die("%s is not hugetlbfs!\n", htlbfs_path);

	blk_size = (unsigned long)sfs.f_bsize;
	if (sfs.f_bsize == 0 || blk_size > size) {
		die("Can't use hugetlbfs pagesize %ld for mem size %lld\n",
			blk_size, (unsigned long long)size);
	}

	kvm->ram_pagesize = blk_size;

	snprintf(mpath, PATH_MAX, "%s/kvmtoolXXXXXX", htlbfs_path);
	fd = mkstemp(mpath);
	if (fd < 0)
		die("Can't open %s for hugetlbfs map\n", mpath);
	unlink(mpath);
	if (ftruncate(fd, size) < 0)
		die("Can't ftruncate for mem mapping size %lld\n",
			(unsigned long long)size);
	addr = mmap(NULL, size, PROT_RW, MAP_PRIVATE, fd, 0);
	close(fd);

	return addr;
}

/* This function wraps the decision between hugetlbfs map (if requested) or normal mmap */
void *mmap_anon_or_hugetlbfs(struct kvm *kvm, const char *hugetlbfs_path, u64 size)
{
	if (hugetlbfs_path)
		/*
		 * We don't /need/ to map guest RAM from hugetlbfs, but we do so
		 * if the user specifies a hugetlbfs path.
		 */
		return mmap_hugetlbfs(kvm, hugetlbfs_path, size);
	else {
		kvm->ram_pagesize = getpagesize();
		return mmap(NULL, size, PROT_RW, MAP_ANON_NORESERVE, -1, 0);
	}
}
