#!/bin/bash
# Objective:
# Boot a virtual Linux system in QEMU with a dummy PCI device that can be
# bound to the uio_pci_generic driver and interacted with from user space.
#
# Find that the simple emulated PCI device 'pci-testdev' does *not* seem to
# correctly bind with the uio_pci_generic driver; so we also emulate another
# PCI device: virtio-serial-pci; this does seem to bind with the
# uio_pci_generic driver.

# Boot into the Qemu VM off it's disk
[[ ! -f ./alpine.img ]] && {
   echo "The Qemu QCOW disk isn't ready; run the qemu_setup_alpine.sh script,
setup the VM, and then run this script"
   exit 1
}

DISK=alpine.img
qemu-system-x86_64 \
  -m 512M \
  -smp 2 \
  -cpu host \
  -enable-kvm \
  -nographic \
  -net nic -net user \
  -drive file="$DISK",format=qcow2 \
  -device pci-testdev \
  -device virtio-serial-pci,id=vs0

#--- Reg the -enable-kvm option:
# Keeping the -enable-kvm is (really) good for performance; however,
# it seems to only support one instance of a KVM-enabled guest on a given
# system. Hence, it fails to boot if you have another hypervisor - like Oracle
# VirtualBox that's also using kvm - running simultaneously.
#---
  
#---
# Once in the guest:
#  Login as root (give the password you specified at installation)
#  Only the first time:
#   apk update
#   apk add pciutils bash
#  modprobe uio_pci_generic

