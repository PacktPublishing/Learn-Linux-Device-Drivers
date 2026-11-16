#!/bin/bash
# common code+data for the bind and restore helper scripts

name=$(basename $0)
die() { echo >&2 "FATAL:${name}: $*" ; exit 1
}
warn() { echo >&2 "WARNING:${name}: $*"
}

DRV=sumouse
ORIG_DRV=usbhid
SYSD=/sys/bus/usb/devices

# Check: HID class == 03 && boot protocol subclass == 01 && mouse proto == 02
is_boot_proto_mouse()
{
	local d=$1
	[[ "$(cat "$d/bInterfaceClass" 2>/dev/null)" = 03 ]] &&
	[[ "$(cat "$d/bInterfaceSubClass" 2>/dev/null)" = 01 ]] &&
	[[ "$(cat "$d/bInterfaceProtocol" 2>/dev/null)" = 02 ]]
}

find_usbmouse_intf()
{
  local f found=0
  export IFS=$'\n'
  for f in $(ls ${SYSD}/)
  do
    #echo "f= ${SYSD}/$f"
    is_boot_proto_mouse "${SYSD}/$f" && {
      found=1
      break
    }
  done
  [[ ${found} -eq 1 ]] && echo "$(basename ${SYSD}/$f)" || echo "XXX"
}
