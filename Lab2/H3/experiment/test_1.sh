#!/bin/bash

DB_PATH="./mydb"
LOG_DIR="./exp_log"
NUM_KEYS=1000000
HOT_START=0
HOT_END=199999
VALUE_SIZE=$((16 * 1024))  # 16KB
HOT_RATIO=70

mkdir -p "$LOG_DIR"

# 실험 조합 리스트
declare -a EXPERIMENTS=(
    "level level"
    "level universal"
    "universal level"
    "universal universal"
)

# 컴파일된 rocksdb_benchmark 실행 파일 이름
EXEC="./rocksdb_benchmark"

for EXP in "${EXPERIMENTS[@]}"; do
    # 공백 분리
    read -r HOT_STYLE COLD_STYLE <<< "$EXP"

    # 로그 파일 이름 지정
    LOG_FILE="$LOG_DIR/hot_${HOT_STYLE}_cold_${COLD_STYLE}.log"

    echo "실험 시작: hot=$HOT_STYLE, cold=$COLD_STYLE"
    echo "로그: $LOG_FILE"

    # 기존 DB 제거
    rm -rf "$DB_PATH"

    # 실험 실행
    "$EXEC" "$DB_PATH" "$NUM_KEYS" "$HOT_START" "$HOT_END" "$VALUE_SIZE" "$HOT_RATIO" "$COLD_STYLE" "$HOT_STYLE" \
        > "$LOG_FILE" 2>&1

    echo "완료됨: $LOG_FILE"
    echo "-------------------------------"
done

echo "모든 실험 완료 ✅"
