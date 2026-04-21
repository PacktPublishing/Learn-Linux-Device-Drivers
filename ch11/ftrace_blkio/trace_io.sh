#!/bin/bash
# Script to run the blk io trace example
# Usage: ./trace_io.sh
# Check if the program is built
# Turn on unofficial Bash 'strict mode'! V useful
# "Convert many kinds of hidden, intermittent, or subtle bugs into immediate, glaringly obvious errors"
# ref: http://redsymbol.net/articles/unofficial-bash-strict-mode/
set -euo pipefail

name=$(basename $0)
die()
{
echo >&2 "FATAL:${name}: $*" ; exit 1
}
warn()
{
echo >&2 "WARNING:${name}: $*"
}
# runcmd
# Parameters
#   $1 ... : params are the command to run
runcmd()
{
	[[ $# -eq 0 ]] && return
	echo "$@"
	eval "$@"
}


#-- 'main'
PUT=file_io_test
which trace-cmd > /dev/null || die "trace-cmd not found. Please install trace-cmd and try again."
if [ ! -f ${PUT} ]; then
    echo "Executable '${PUT}' not found. Please build the program first using 'make'."
    exit 1
fi
TRC_CMD=$(which trace-cmd)
# Run the trace
echo "sudo ${TRC_CMD} record -o trccmd_trc_io.dat \
    -q -p function_graph \
    -e syscalls -e block -e ext4 -e filemap -e irq -e nvme -e writeback \
    -c \
    -F ./${PUT}"
  #  -l submit_bio -l blk_mq_* 
sudo ${TRC_CMD} record -o trccmd_trc_io.dat \
    -q -p function_graph \
    -e syscalls -e block -e ext4 -e filemap -e irq -e nvme -e writeback \
    -c \
    -F ./${PUT}
    # event classes to trace; update as required \
    # particular functions to trace; update as required \
    #-l submit_bio -l blk_mq_*  

# Generate the trace report
echo "sudo ${TRC_CMD} report -l -i trccmd_trc_io.dat -q > /tmp/trc.txt"
sudo ${TRC_CMD} report -l -i trccmd_trc_io.dat -q > /tmp/trc.txt
echo "Trace report generated at /tmp/trc.txt"
ls -lh /tmp/trc.txt