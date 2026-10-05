#!/usr/bin/env bash
set -euo pipefail
# Usage: ./run_all.sh /path/to/ชุดข้อมูลทดสอบ_01204212
if [[ $# -ne 1 ]]; then
  echo 'Usage: ./run_all.sh /path/to/ชุดข้อมูลทดสอบ_01204212' >&2
  exit 1
fi
DATA_DIR="$1"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo '[1/2] Compiling timing_avl.c ...'
gcc timing_avl.c -o timing_avl

echo '[2/2] Running n=1,000 ...'
./timing_avl "$DATA_DIR/data_1000.txt" "$DATA_DIR/targets_1000.txt"
echo '[2/2] Running n=10,000 ...'
./timing_avl "$DATA_DIR/data_10000.txt" "$DATA_DIR/targets_10000.txt"
echo '[2/2] Running n=100,000 ...'
./timing_avl "$DATA_DIR/data_100000.txt" "$DATA_DIR/targets_100000.txt"
echo 'All timing runs completed successfully.'
