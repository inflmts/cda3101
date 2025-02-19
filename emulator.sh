#!/bin/sh

export MSYS2_ARG_CONV_EXCL='*'
exec qemu-system-aarch64 \
  -smp 2 -m 1G -M virt -cpu cortex-a57 \
  -initrd alpine/initramfs-virt \
  -kernel alpine/vmlinuz-virt \
  -append 'alpine_repo=https://dl-cdn.alpinelinux.org/alpine/latest-stable/main' \
  -drive file=alpine/disk.qcow2,format=qcow2,cache=unsafe,if=virtio \
  -device e1000,netdev=net \
  -netdev user,id=net,hostfwd=tcp:127.0.0.1:3101-:22 \
  -nographic
