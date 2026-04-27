#!/bin/bash
# Credit: https://oneuptime.com/blog/post/2026-03-02-use-fio-io-performance-benchmarking-ubuntu/view
# quick_storage_benchmark.sh - Run standard storage tests and summarize

TESTFILE="/tmp/fio_benchmark_$$"
RUNTIME=30  # Seconds per test

echo "=== Storage Benchmark ==="
echo "Date: $(date)"
echo "Hostname: $(hostname)"
echo "Test file: $TESTFILE"
echo ""

run_test() {
    local name="$1"
    local rw="$2"
    local bs="$3"
    local numjobs="$4"
    local iodepth="$5"

    # Change --ioengine from 'libaio' to 'io_uring'
    result=$(fio \
        --name="$name" \
        --ioengine=io_uring \
        --rw="$rw" \
        --bs="$bs" \
        --direct=1 \
        --numjobs="$numjobs" \
        --iodepth="$iodepth" \
        --size=2G \
        --filename="$TESTFILE" \
        --runtime="$RUNTIME" \
        --time_based \
        --output-format=json \
        --group_reporting 2>/dev/null)
       # --ioengine=libaio

    read_bw=$(echo "$result" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d['jobs'][0]['read']['bw_bytes']//1024//1024)" 2>/dev/null)
    read_iops=$(echo "$result" | python3 -c "import sys,json; d=json.load(sys.stdin); print(int(d['jobs'][0]['read']['iops']))" 2>/dev/null)
    write_bw=$(echo "$result" | python3 -c "import sys,json; d=json.load(sys.stdin); print(d['jobs'][0]['write']['bw_bytes']//1024//1024)" 2>/dev/null)
    write_iops=$(echo "$result" | python3 -c "import sys,json; d=json.load(sys.stdin); print(int(d['jobs'][0]['write']['iops']))" 2>/dev/null)

    printf "%-25s Read: %6d MB/s  %8d IOPS  |  Write: %6d MB/s  %8d IOPS\n" \
        "$name:" "${read_bw:-0}" "${read_iops:-0}" "${write_bw:-0}" "${write_iops:-0}"
}

run_test "Seq Read (1M bs)" "read" "1M" 1 8
run_test "Seq Write (1M bs)" "write" "1M" 1 8
run_test "Rand Read 4K" "randread" "4K" 4 64
run_test "Rand Write 4K" "randwrite" "4K" 4 64
run_test "Mixed 70/30 4K" "randrw" "4K" 4 32

# Clean up test file
rm -f "$TESTFILE"
echo ""
echo "Benchmark complete"
