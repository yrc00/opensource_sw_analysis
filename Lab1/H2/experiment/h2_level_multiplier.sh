#!/bin/bash

# 워크로드 종류
workloads=("fillrandom" "overwrite")

# 컴팩션 종류
compaction_style=0

# 데이터 사이즈
value_sizes=(1024 4096 16384)  # 1KB, 4KB, 16KB

# DB 엔트리 수
num=1000000

# Leveld compaction 설정
max_bytes_for_level_base=134217728  # 128MB
max_bytes_for_level_multiplier=(4, 10, 20)

# db_bench 경로
DB_BENCH="./db_bench"

# 로그 디렉토리 설정
log_dir="exp_logs"
mkdir -p "$log_dir"

# 실험 루프
for val_size in "${value_sizes[@]}"; do
  for workload in "${workloads[@]}"; do
    for level_multiplier in "${max_bytes_for_level_multiplier[@]}"; do

      log_file="${log_dir}/h1_${workload}_cs${compaction_style}_size${val_size}_multiplier${level_multiplier}.log"
      db_path="/tmp/rocksdb_h1_${workload}_cs${compaction_style}_size${val_size}_multiplier${level_multiplier}"

      echo "===== Running ${workload} | Value_size=${val_size} | Level_multiplier=${level_multiplier} ====="
      echo "Logging to $log_file"

      rm -rf "$db_path"

      $DB_BENCH \
        --benchmarks=$workload \
        --value_size=$val_size \
        --max_bytes_for_level_base=$max_bytes_for_level_base \
        --max_bytes_for_level_multiplier=$level_multiplier \
        --num=$num \
        --compaction_style=$compaction_style \
        --statistics \
        --db="$db_path" \
        > "$log_file" 2>&1

      echo "Done: $log_file"
      echo
    done
  done
done

echo "모든 실험이 완료되었습니다."
