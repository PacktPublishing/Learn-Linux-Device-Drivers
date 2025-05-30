#!/bin/bash
# Here we ASSUME that:
#  The emulated virtio PCI device's BDF # (Bus:Dev:Func) is [0000:]00:05.0
#  and it's [VID,PID] is [1af4,1003]

#--- KEEP UPDATED ---
PCI_DEVNAME=virtio
PCI_DRIVER=virtio-pci
PCI_BDF="00:05.0"
PCI_VID_PID="1af4 1003"

runcmd()
{
	[[ $# -eq 0 ]] && return
	echo "$@"
	eval "$@"
	echo
}

# reset
runcmd "echo -n "0000:${PCI_BDF}" > /sys/bus/pci/drivers/uio_pci_generic/unbind"

runcmd "lspci -nnk|grep -i -A3 ${PCI_DEVNAME}"
runcmd "modprobe uio_pci_generic"

runcmd "echo "${PCI_VID_PID}" > /sys/bus/pci/drivers/uio_pci_generic/new_id"
runcmd "echo -n "0000:${PCI_BDF}" > /sys/bus/pci/drivers/${PCI_DRIVER}/unbind"
runcmd "echo -n "0000:${PCI_BDF}" > /sys/bus/pci/drivers/uio_pci_generic/bind"

# Verify
runcmd "ls -l /sys/bus/pci/devices/0000:${PCI_BDF}/driver"
runcmd "ls -l /dev/uio*"
