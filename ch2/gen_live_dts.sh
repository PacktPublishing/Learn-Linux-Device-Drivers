#!/bin/bash

set -euo pipefail
name=$(basename $0)

which dtc >/dev/null 2>&1 || {
  echo "${name}: dtc not installed? Aborting..." ; exit 1
}
[[ ! -d /proc/device-tree ]] && {
  echo "${name}: /proc/device-tree not present? Ensure you're running this on a target that using a DT. Aborting..."
  exit 1
}
echo "dtc -I fs -@ -O dts /proc/device-tree -o /tmp/live.dts 2>/dev/null"
dtc -I fs -@ -O dts /proc/device-tree -o /tmp/live.dts 2>/dev/null
