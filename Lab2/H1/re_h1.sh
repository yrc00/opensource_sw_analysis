#!/bin/bash

# 컴팩션 스타일: 0 = level, 1 = universal, 2 = FIFO
compaction_styles=(0 1 2)

# 워크로드 종류
workloads=("fillrandom" "overwrite" "readwhilewriting")

# 설정: 성능 최적화 / 안정성 위주
declare -A configs
configs["perf"]="-disable_wal=true -sync=false -use_direct_io_for_flush_and_compaction=false"
# stable에서 sync=true로 설정 시 실험이 끝나지 않음(매우 오래 걸림)
configs["stable"]="-disable_wal=false -sync=false -use_direct_io_for_flush_and_compaction=false"

# 공통 설정
value_size=16384 #16384
num=100000 # 100만 사용 시 11번 실험에서 진행이 매우 오래걸림

# db_bench 경로 (필요시 수정)
DB_BENCH="./db_bench"

# 로그 디렉토리 설정
log_dir="exp_logs"
mkdir -p "$log_dir"

# 실험 루프
for comp_style in "${compaction_styles[@]}"; do
  for workload in "${workloads[@]}"; do
    for config_type in "perf" "stable"; do

      log_file="${log_dir}/${workload}_cs${comp_style}_${config_type}.log"
      db_path="/tmp/rocksdb_${workload}_cs${comp_style}_${config_type}"

      echo "===== Running ${workload} | CompactionStyle=${comp_style} | Config=${config_type} ====="
      echo "Logging to $log_file"

      rm -rf "$db_path"

      $DB_BENCH \
        -benchmarks=$workload \
        -value_size=$value_size \
        -num=$num \
        -compaction_style=$comp_style \
        -statistics \
        -db="$db_path" \
        ${configs[$config_type]} \
        > "$log_file" 2>&1

      echo "Done: $log_file"
      echo
    done
  done
done

echo "모든 실험이 완료되었습니다."
