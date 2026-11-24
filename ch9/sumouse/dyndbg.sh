#!/bin/bash
# DD = Dyn Debug !

# Turn on unofficial Bash 'strict mode'! V useful
# "Convert many kinds of hidden, intermittent, or subtle bugs into immediate, glaringly obvious errors"
# ref: http://redsymbol.net/articles/unofficial-bash-strict-mode/
set -euo pipefail

name=$(basename $0)
die() { echo >&2 "FATAL:${name}: $*" ; exit 1
}
warn() { echo >&2 "WARNING:${name}: $*"
}

DDCTL=/proc/dynamic_debug/control
DRV=sumouse

state() {
echo -n "Current dynamic debug state for ${DRV}: "
grep "${DRV}" ${DDCTL} |grep "=_" >/dev/null && echo "OFF"
grep "${DRV}" ${DDCTL} |grep "=p" >/dev/null && echo "ON"
grep "${DRV}" ${DDCTL}
}
usage() {
cat << EOF
Usage: ${name} [option]
 option = 1 : turn  ON this driver's dyn debug printk's
 option = 0 : turn OFF this driver's dyn debug printk's

$(state)
EOF
}


#-- 'main'
lsmod |grep ${DRV} >/dev/null 2>&1 || {
   echo "Driver \"${DRV}\" isn't loaded, pl load it first..."
   exit 1
}
[[ $# -ne 1 ]] && {
   usage ; exit 1
}

case "$1" in
 1) echo "Turning dynamic debug ON now"
    sudo sh -c "echo \"module ${DRV} +p\" > ${DDCTL}"
    state
    ;;
 0) echo "Turning dynamic debug OFF now"
    sudo sh -c "echo \"module ${DRV} -p\" > ${DDCTL}"
    state
    ;;
 *) usage ; exit 1
    ;;
esac
exit 0
