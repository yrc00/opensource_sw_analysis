#!/bin/bash

DB_PATH="./mydb"
LOG_DIR="./exp_log"
NUM_KEYS=1000000
HOT_START=0
HOT_END=199999
VALUE_SIZE=$((16 * 1024))  # 16KB
HOT_RATIO=70

mkdir -p "$LOG_DIR"

# 실험 조합 리스트: "hot_compression cold_compression"
declare -a EXPERIMENTS=(
    "none ZSTD"
    "none Zlib"
    "LZ4 ZSTD"
    "LZ4 Zlib"
    "Snappy ZSTD"
    "Snappy Zlib"
)

# 실행 파일 이름
EXEC="./rocksdb_benchmark"

# 고정 컴팩션 방식
HOT_COMPACTION="level"
COLD_COMPACTION="universal"

for EXP in "${EXPERIMENTS[@]}"; do
    read -r HOT_COMPRESSION COLD_COMPRESSION <<< "$EXP"

    # 로그 파일 이름에는 압축 방식만 포함
    LOG_FILE="$LOG_DIR/hot_${HOT_COMPRESSION}_cold_${COLD_COMPRESSION}.log"

    echo "실험 시작: hot=($HOT_COMPRESSION), cold=($COLD_COMPRESSION)"
    echo "로그: $LOG_FILE"

    # 기존 DB 제거
    rm -rf "$DB_PATH"

    # 실행
    "$EXEC" "$DB_PATH" "$NUM_KEYS" "$HOT_START" "$HOT_END" "$VALUE_SIZE" "$HOT_RATIO" \
        "$COLD_COMPACTION" "$HOT_COMPACTION" "$COLD_COMPRESSION" "$HOT_COMPRESSION" \
        > "$LOG_FILE" 2>&1

    echo "완료됨: $LOG_FILE"
    echo "-------------------------------"
done

echo "모든 실험 완료 ✅"
