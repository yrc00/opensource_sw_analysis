#!/bin/bash

# 워크로드 종류
workloads=("fillrandom" "overwrite")

# 컴팩션 종류
compaction_style=1

# 데이터 사이즈
value_sizes=(1024 4096 16384)  # 1KB, 4KB, 16KB

# DB 엔트리 수
num=1000000

# db_bench 경로
DB_BENCH="./db_bench"

# 로그 디렉토리 설정
log_dir="exp_logs"
mkdir -p "$log_dir"

# 실험 루프
for val_size in "${value_sizes[@]}"; do
  for workload in "${workloads[@]}"; do

    log_file="${log_dir}/h1_${workload}_cs${compaction_style}_size${val_size}.log"
    db_path="/tmp/rocksdb_h1_${workload}_cs${compaction_style}_size${val_size}"

    echo "===== Running ${workload} | Value_size=${val_size}} ====="
    echo "Logging to $log_file"

    rm -rf "$db_path"

    $DB_BENCH \
      --benchmarks=$workload \
      --value_size=$val_size \
      --num=$num \
      --compaction_style=$compaction_style \
      --statistics \
      --db="$db_path" \
      > "$log_file" 2>&1

    echo "Done: $log_file"
    echo
  done
done

echo "모든 실험이 완료되었습니다."
