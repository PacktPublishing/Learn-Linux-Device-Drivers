#!/bin/bash

# Objective:
# Boot a virtual Linux system in QEMU with a dummy PCI device that can be
# bound to the uio_pci_generic driver and interacted with from user space.
#
# First time setup
# Boot into the Qemu VM via the 'live CD' ISO image, and then set it up

first_time_setup()
{
  wget https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/x86_64/alpine-standard-3.20.0-x86_64.iso -O alpine.iso
  qemu-img create -f qcow2 alpine.img 2G
}

#--- 'main'
[[ ! -f ./alpine.iso ]] && first_time_setup

ISO=alpine.iso
DISK=alpine.img

QEMU_CMD="qemu-system-x86_64 \
  -m 512M \
  -smp 2 \
  -enable-kvm \
  -cpu host \
  -nographic \
  -net nic -net user \
  -drive file="${DISK}",format=qcow2 \
  -cdrom "${ISO}" \
  -boot once=d \
  -device pci-testdev"

# No KVM? If so, lose the -enable-kvm & -cpu host options
lsmod | egrep -w "kvm-intel|kvm-amd" >/dev/null || \
 QEMU_CMD="qemu-system-x86_64 \
  -m 512M \
  -smp 2 \
  -nographic \
  -net nic -net user \
  -drive file="${DISK}",format=qcow2 \
  -cdrom "${ISO}" \
  -boot once=d \
  -device pci-testdev"

echo "${QEMU_CMD}"
eval "${QEMU_CMD}"

#---
# Once in the guest, do these steps:
#  Login as root (no password)
#  setup-alpine
#  - Choose eth0 as your network
#  - Use UTC or your timezone
#  - Choose to install Alpine to disk (sda)
#  - Use sys mode (normal installation)
#  - Allow it to format disk
#  - Choose to install the openssh package (optional)
#  - Accept reboot
#
# Run the boot_alpine.sh script
#---
