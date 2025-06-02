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

# 실행 파일
WRITE_EXEC="./rocksdb_benchmark"
READ_EXEC="./rocksdb_read_benchmark"  # ← 아까 만든 읽기 전용 벤치마크

for EXP in "${EXPERIMENTS[@]}"; do
    read -r HOT_STYLE COLD_STYLE <<< "$EXP"

    # 로그 파일 지정
    WRITE_LOG="$LOG_DIR/write_hot_${HOT_STYLE}_cold_${COLD_STYLE}.log"
    READ_LOG="$LOG_DIR/read_hot_${HOT_STYLE}_cold_${COLD_STYLE}.log"

    echo "쓰기 실험 시작: hot=$HOT_STYLE, cold=$COLD_STYLE"
    echo "→ 로그: $WRITE_LOG"

    rm -rf "$DB_PATH"

    "$WRITE_EXEC" "$DB_PATH" "$NUM_KEYS" "$HOT_START" "$HOT_END" "$VALUE_SIZE" "$HOT_RATIO" "$COLD_STYLE" "$HOT_STYLE" \
        > "$WRITE_LOG" 2>&1

    echo "읽기 실험 시작: hot=$HOT_STYLE, cold=$COLD_STYLE"
    echo "→ 로그: $READ_LOG"

    "$READ_EXEC" "$DB_PATH" "$NUM_KEYS" "$HOT_START" "$HOT_END" "$VALUE_SIZE" "$HOT_RATIO" "$COLD_STYLE" "$HOT_STYLE" \
        > "$READ_LOG" 2>&1

    echo "완료됨: hot=$HOT_STYLE, cold=$COLD_STYLE"
    echo "-------------------------------"
done

echo "모든 실험 완료 ✅"