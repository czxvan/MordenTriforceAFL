#!/bin/sh
../../qemu_mode/qemu-8.2.0/build-arm/qemu-system-arm \
    -M vexpress-a9 -smp 1 -m 256 \
    -kernel kern/zImage -dtb kern/vexpress-v2p-ca9.dtb \
    -drive file=rootfs.ext2,if=sd,format=raw \
    -append "console=ttyAMA0,115200 rootwait root=/dev/mmcblk0 init=/init" \
    -net nic,model=lan9118 -net user -nographic
