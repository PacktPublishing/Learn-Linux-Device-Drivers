#!/bin/bash
# Create a demo USB multi-interface gadget device via ConfigFS

# Turn on unofficial Bash 'strict mode'! V useful
# "Convert many kinds of hidden, intermittent, or subtle bugs into immediate, glaringly obvious errors"
# ref: http://redsymbol.net/articles/unofficial-bash-strict-mode/
set -euo pipefail

name=$(basename $0)
die() { echo >&2 "FATAL:${name}: $*" ; exit 1
}
warn() { echo >&2 "WARNING:${name}: $*"
}

GADGET=g1

clear_gadget()
{
set +e
[[ -e  /sys/kernel/config/usb_gadget/${GADGET} ]] && cd /sys/kernel/config/usb_gadget/${GADGET} || return
echo "" > UDC                    # unplug
rm configs/c.1/acm.usb0
rmdir configs/c.1/strings/0x409
rmdir configs/c.1
rmdir functions/acm.usb0
rmdir strings/0x409
cd .. && rmdir ${GADGET}
set -e
}


#-- 'main'

# verify kernel config
echo "This kernel ver: $(uname -r)"
grep -E -w "CONFIG_USB_SUPPORT=y|CONFIG_USB_GADGET=[ym]|CONFIG_USB_CONFIGFS=[ym]|CONFIG_USB_DUMMY_HCD=[ym]" /boot/config-$(uname -r)
grep -E -w "CONFIG_USB_SUPPORT=y|CONFIG_USB_GADGET=[ym]|CONFIG_USB_CONFIGFS=[ym]|CONFIG_USB_DUMMY_HCD=[ym]" /boot/config-$(uname -r) |grep "not set" >/dev/null 2>&1 && die "Requires USB_SUPPORT=y USB_GADGET=y|m, USB_CONFIGFS=y|m, CONFIG_USB_DUMMY_HCD=y|m"
echo

# other checks
which usb-devices >/dev/null 2>&1 || die "Pl install the usb-devices utility first"
[[ $(id -u) -ne 0 ]] && die "needs root."

# Setup. (Load the gadget core; this creates /sys/kernel/config/usb_gadget/)
modprobe libcomposite
modprobe dummy_hcd
cd /sys/kernel/config/usb_gadget
clear_gadget

echo "--- Before gadget creation: usb-devices (looking for \"Manufacturer=LDDIA Test Labs\")"
#lsusb -tv |grep -C3 -i "Multifunction Composite Gadget" || true
usb-devices | grep -B3 -A8  "Manufacturer=LDDIA Test Labs" || true
echo

echo "> Creating gadget via ConfigFS now"
# Gadget: setup device Descriptor
mkdir ${GADGET} && cd ${GADGET}        # kernel creates idVendor, strings/, configs/, functions/, UDC ...
echo 0x1d6b > idVendor   # Linux Foundation VID: fine for a demo never otherwise
echo 0x0104 > idProduct

mkdir strings/0x409                    # 0x409 = US English
echo 0001        > strings/0x409/serialnumber
echo "LDDIA Test Labs" > strings/0x409/manufacturer
echo "Gadget ACM demo"  > strings/0x409/product

# Setup a Configuration descriptor
mkdir configs/c.1
mkdir configs/c.1/strings/0x409
echo "ACM config" > configs/c.1/strings/0x409/configuration
echo 120 > configs/c.1/MaxPower        # mA

# Here's the cool part! Setup a multi-interface USB gadget, one with two
# 'functions' (a function is equivalent to an Interface + it's Endpoints).
# This setup autoloads the usb_f_acm function driver!
#  Config #1: If #0 : Class: Communications (CDC), AT-commands : 1 EP
#                      EP 1 : Interrupt IN
#  Config #1: If #1 : Class: CDC Data : 2 EPs
#                      EP 1 : Bulk IN
#                      EP 2 : Bulk OUT
mkdir functions/acm.usb0               # auto-loads usb_f_acm
# Put the function in the configuration
ln -s functions/acm.usb0 configs/c.1/

# Bind to the UDC : 'plug in the cable'
echo dummy_udc.0 > UDC

echo "Done."
echo "ls -l /sys/kernel/config/usb_gadget/${GADGET}/"
ls -l /sys/kernel/config/usb_gadget/${GADGET}/

echo "
--- After gadget creation: usb-devices (looking for \"Manufacturer=LDDIA Test Labs\")"
usb-devices | grep -B3 -A8  "Manufacturer=LDDIA Test Labs" || true
echo "
Lookup: lsusb -v output, and the kernel log as well."
exit 0
