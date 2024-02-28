#!/bin/sh
../../qemu_mode/qemu-8.2.0/build-x86_64/qemu-system-x86_64 \
    -L ../../qemu_mode/qemu-8.2.0/pc-bios \
    -kernel kern/bzImage -initrd ./fuzzRoot.cpio.gz \
    -m 64M -nographic -append "console=ttyS0"
