#!/bin/bash
# Helper script for setting up the (new) block driver on x86_64.
# We load up the driver, it creates virtual disks; we format an ext4 filesystem
# on the first virtual disk.
# NOTE: we simply ASSUME that /mnt/tmp is unused.
#
# Kaiwan N Billimoria, kaiwanTECH
# License: GPL

# Turn on Bash 'strict mode'!
# ref: http://redsymbol.net/articles/unofficial-bash-strict-mode/
set -euo pipefail

name=$(basename $0)
DRVNAME=sblkdev
DISKNAME=sblkdev1
MOUNTPT=/mnt/tmp   # NOTE: we simply ASSUME that /mnt/tmp is unused

if [ $(id -u) -ne 0 ]; then
  echo "${name}: must run as root." ; exit 1
fi

umount ${MOUNTPT} 2>/dev/null || true
mkdir -p ${MOUNTPT} 2>/dev/null || true
rmmod ${DRVNAME} 2>/dev/null || true
dmesg -C
[[ ! -f ${DRVNAME}.ko ]] && {
	./mk.sh build || exit 1
}
# use defaults for the 'disks': hard-coded as "sblkdev1,4096;sblkdev2,8192"
insmod ./${DRVNAME}.ko || exit 1
dmesg
# dev nodes auto-created...

# We DON'T require to fdisk to create a partition; in fact doing this directly
# contradicts with the driver's GENHD_FL_NO_PART flag.. so we skip it.
# The mkfs.ext4 will work upon the whole disk (that's fine).

echo "${name} : -----Creating ext4 filesystem now ..."
mkfs.ext4 /dev/${DISKNAME}

echo "${name} : -----Mounting new ext4 filesystem now..."
[[ ! -d ${MOUNTPT} ]] && mkdir -p ${MOUNTPT}
mount /dev/${DISKNAME} ${MOUNTPT} || exit 1
mount |grep ${DISKNAME}

echo "${name} : disk '${DISKNAME}' ready, try it out!"
exit 0
