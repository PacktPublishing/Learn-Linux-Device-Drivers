#!/bin/bash

show_disk_detail()
{
    local DEV=${1} hctx
    printf "# hctx's    (nr_hw_queues)  : %4d\n" $(ls /sys/block/${DEV}/mq/ | wc -w)
    printf "queue depth (nr_tags)       : %4d\n" $(cat /sys/block/${DEV}/mq/0/nr_tags)
    printf "hctx#     nr_tags     CPU affinity list\n"
    for hctx in /sys/block/${DEV}/mq/*/; do
        #echo ">>> hctx = $hctx"
        printf "%4s  %9s   " $(basename ${hctx}) $(cat ${hctx}/nr_tags) 
        echo "      $(cat ${hctx}/cpu_list)"
    done
    echo "iosched: $(cat /sys/block/${DEV}/queue/scheduler)"
}

#--- 'main' ---
for DEV in $(ls /sys/block/ | grep -E 'nvme|sd|sblkdev'); do
    echo "---------------------------------------"
    echo "Device: ${DEV}"
    echo "---------------------------------------"
    show_disk_detail ${DEV}
    echo
done
echo "# hctx's    : # of hardware I/O queues : for NVMe, the number of CPU cores (driver sets it).
Queue depth : number of in-flight I/O requests possible within a single hardware hctx."