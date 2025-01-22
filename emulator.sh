#!/bin/sh

root=qemu-arm-img
ncpu=2
mem=1G

export MSYS2_ARG_CONV_EXCL='*'
exec qemu-system-aarch64 \
  -smp "$ncpu" -m "$mem" -M virt -cpu cortex-a57  \
  -initrd "$root/initrd.img-4.9.0-4-arm64" \
  -kernel "$root/vmlinuz-4.9.0-4-arm64" -append "root=/dev/sda2 console=ttyAMA0" \
  -global virtio-blk-device.scsi=off \
  -device virtio-scsi-device,id=scsi \
  -drive file="$root/disk.qcow2",id=rootimg,cache=unsafe,if=none \
  -device scsi-hd,drive=rootimg \
  -device e1000,netdev=net0 \
  -netdev user,id=net0,hostfwd=tcp:127.0.0.1:3101-:22 \
  -nic user,model=virtio-net-pci \
  -nographic -snapshot
