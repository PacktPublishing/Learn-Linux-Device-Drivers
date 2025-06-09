#!/bin/bash
# Here we ASSUME that:
#  The emulated virtio PCI device's BDF # (Bus:Dev:Func) is [0000:]00:05.0
#  and it's [VID,PID] is [1af4,1003]

#--- KEEP UPDATED ---
PCI_DEVNAME="virtio"
PCI_DRIVER=virtio-pci
PCI_BDF="00:05.0"
PCI_VID_PID="1af4:1003"

# As another example, the Ethernet controller PCI device
# NOTE:
# - Doing this on an existing 'live' device can be dangerous! It will almost
#   certainly cause it to stop functioning or misbehave... (it's why we only
#   run this experiment on a Qemu-emulated 'dummy' PCIe device, one we aren't
#   actively using)
# - Update the below values to reflect lspci -nnv output on your box
#PCI_DEVNAME="Ethernet controller"
#PCI_DRIVER=e1000
#PCI_BDF="00:03.0"
#PCI_VID_PID="8086:100e"

die()
{
echo >&2 "FATAL: $*" ; exit 1
}
runcmd()
{
	[[ $# -eq 0 ]] && return
	echo "$@"
	eval "$@"
	echo
}

#--- 'main'
[[ $(id -u) -ne 0 ]] && die "Need root"
hash lspci || die "Install lspci util"
# Does the PCI device exist?
lspci -nn | grep "${PCI_BDF}.*${PCI_VID_PID}" >/dev/null || die "No PCI device matching address \"${PCI_BDF}\" and [VID:PID] \"${PCI_VID_PID}\""

# reset
runcmd "echo -n "0000:${PCI_BDF}" > /sys/bus/pci/drivers/uio_pci_generic/unbind 2>/dev/null"

runcmd "lspci -nnk|grep -i -A3 ${PCI_DEVNAME}"
runcmd "modprobe uio_pci_generic 2>/dev/null"

PCI_VID_PID_SPACESEP=$(echo ${PCI_VID_PID} | sed -e "s/:/ /")
runcmd "echo "${PCI_VID_PID_SPACESEP}" > /sys/bus/pci/drivers/uio_pci_generic/new_id"
# Unbind PCI device from the original driver
runcmd "echo -n "0000:${PCI_BDF}" > /sys/bus/pci/drivers/${PCI_DRIVER}/unbind"
# Bind PCI device to the kernel uio_pci_generic driver
runcmd "echo -n "0000:${PCI_BDF}" > /sys/bus/pci/drivers/uio_pci_generic/bind"

# Verify
runcmd "ls -l /sys/bus/pci/devices/0000:${PCI_BDF}/driver"
runcmd "ls -l /dev/uio*"
