#include <linux/stringify.h>

#ifndef KVM__UTIL_H
#define KVM__UTIL_H

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

/*
 * Some bits are stolen from perf tool :)
 */

#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <sys/param.h>
#include <sys/types.h>
#include <linux/types.h>

#ifdef __GNUC__
#define NORETURN __attribute__((__noreturn__))
#else
#define NORETURN
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

extern bool do_debug_print;

#define PROT_RW (PROT_READ|PROT_WRITE)
#define MAP_ANON_NORESERVE (MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE)

extern void die(const char *err, ...) NORETURN __attribute__((format (printf, 1, 2)));
extern void die_perror(const char *s) NORETURN;
extern int pr_err(const char *err, ...) __attribute__((format (printf, 1, 2)));
extern void pr_warning(const char *err, ...) __attribute__((format (printf, 1, 2)));
extern void pr_info(const char *err, ...) __attribute__((format (printf, 1, 2)));
extern void set_die_routine(void (*routine)(const char *err, va_list params) NORETURN);

#define pr_debug(fmt, ...)						\
	do {								\
		if (do_debug_print)					\
			pr_info("(%s) %s:%d: " fmt, __FILE__,		\
				__func__, __LINE__, ##__VA_ARGS__);	\
	} while (0)


#define BUILD_BUG_ON(condition)	((void)sizeof(char[1 - 2*!!(condition)]))

#ifndef BUG_ON_HANDLER
# define BUG_ON_HANDLER(condition)					\
	do {								\
		if ((condition)) {					\
			pr_err("BUG at %s:%d", __FILE__, __LINE__);	\
			raise(SIGABRT);					\
		}							\
	} while (0)
#endif

#define BUG_ON(condition)	BUG_ON_HANDLER((condition))

#define DIE_IF(cnd)						\
do {								\
	if (cnd)						\
	die(" at (" __FILE__ ":" __stringify(__LINE__) "): "	\
		__stringify(cnd) "\n");				\
} while (0)

#define WARN_ON(condition) ({					\
	int __ret_warn_on = !!(condition);			\
	if (__ret_warn_on)					\
		pr_warning("(%s) %s:%d: failed condition: %s",	\
				__FILE__, __func__, __LINE__,	\
				__stringify(condition));	\
	__ret_warn_on;						\
})

#define MSECS_TO_USECS(s) ((s) * 1000)

/* Millisecond sleep */
static inline void msleep(unsigned int msecs)
{
	usleep(MSECS_TO_USECS(msecs));
}

/*
 * Find last (most significant) bit set. Same implementation as Linux:
 * fls(0) = 0, fls(1) = 1, fls(1UL << 63) = 64
 */
static inline int fls_long(unsigned long x)
{
	return x ? sizeof(x) * 8 - __builtin_clzl(x) : 0;
}

static inline unsigned long roundup_pow_of_two(unsigned long x)
{
	return x ? 1UL << fls_long(x - 1) : 0;
}

#define is_power_of_two(x)	((x) > 0 ? ((x) & ((x) - 1)) == 0 : 0)

/**
 * pow2_size: return the number of bits needed to store values
 * @x: number of distinct values to store (or number of bytes)
 *
 * Determines the number of bits needed to store @x different values.
 * Could be used to determine the number of address bits needed to
 * store @x bytes.
 *
 * Example:
 * pow2_size(255) => 8
 * pow2_size(256) => 8
 * pow2_size(257) => 9
 *
 * Return: number of bits
 */
static inline int pow2_size(unsigned long x)
{
	if (x <= 1)
		return x;

	return sizeof(x) * 8 - __builtin_clzl(x - 1);
}

struct kvm;
void *mmap_hugetlbfs(struct kvm *kvm, const char *htlbfs_path, u64 size);
void *mmap_anon_or_hugetlbfs(struct kvm *kvm, const char *hugetlbfs_path, u64 size);

int setup_debug_socket(const char *socket_path);

#endif /* KVM__UTIL_H */
