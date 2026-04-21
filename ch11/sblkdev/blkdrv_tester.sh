#!/bin/bash
# simple test script for sbd driver
# kaiwan n bill, kaiwanTECH

# Turn on Bash 'strict mode'!
# ref: http://redsymbol.net/articles/unofficial-bash-strict-mode/
set -euo pipefail

# runcmd
# Parameters
#   $1 ... : params are the command to run
runcmd()
{
[ $# -eq 0 ] && return
echo "$@"
eval "$@"
}


name=$(basename $0)
DISK=sblkdev1
MOUNTPT=$(mount |grep ${DISK}|awk '{print $3}' || true)
if [ -z "${MOUNTPT}" ]; then
	echo "${name}: disk ${DISK} not mounted?
Tip: run the loadblk.sh script first..."
	exit 1
fi
if [ ! -d ${MOUNTPT} ]; then
	echo "${name}: mount point ${MOUNTPT} invalid?
Tip: run the loadblk.sh script first..."
	exit 1
fi
sudo dmesg -C
df -h|grep ${DISK}

runcmd "sudo dd if=/dev/urandom of=${MOUNTPT}/t1 bs=512 count=3"
runcmd "sudo dd if=/dev/urandom of=${MOUNTPT}/t2 bs=2k count=1"
runcmd "sudo dd if=/dev/urandom of=${MOUNTPT}/t3 bs=4k count=1"

runcmd "sleep 3 ; sync"

ls -la ${MOUNTPT}
df -h|grep ${DISK}
exit 0
