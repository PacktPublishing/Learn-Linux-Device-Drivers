#!/bin/bash

show_disk_detail()
{
    local DEV=${1} hctx qdepth
    printf "# hctx's    (nr_hw_queues)  : %4d\n" $(ls /sys/block/${DEV}/mq/ | wc -w)
    qdepth=$(cat /sys/block/${DEV}/mq/0/nr_tags)
    qdepth=$((qdepth+1)) # for the Admin Queue pair
    printf "queue depth (nr_tags)       : %4d\n" ${qdepth}
    printf "hctx#     nr_tags     CPU affinity list\n"
    for hctx in /sys/block/${DEV}/mq/*/; do
        printf "%4s  %9s   " $(basename ${hctx}) $(cat ${hctx}/nr_tags) 
        echo "      $(cat ${hctx}/cpu_list)"
    done
    echo "iosched: $(cat /sys/block/${DEV}/queue/scheduler)"
}

#--- 'main' ---
#for DEV in $(ls /sys/block/ | grep -E 'nvme|sd|sblkdev'); do
for DEV in $(\ls /sys/block/ | grep -v 'loop'); do
    echo "---------------------------------------"
    echo "Device: ${DEV}"
    echo "---------------------------------------"
    show_disk_detail ${DEV}
    echo
done
echo "# hctx's    : # of hardware I/O queues : for NVMe, the number of CPU cores (driver sets it).
Queue depth : number of in-flight I/O requests possible within a single hardware hctx."
