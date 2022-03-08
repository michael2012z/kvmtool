sudo ../lkvm run --cpus 2 --disk ./hello-rootfs.ext4 --kernel ~/ws/src/github.com/michael2012z/myLinuxGuest/arch/arm64/boot/Image --force-pci --irqchip gicv3 --debug --dump-dtb test.dtb

