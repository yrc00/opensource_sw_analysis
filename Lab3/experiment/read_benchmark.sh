#!/bin/bash

DB_PATH="./mydb"
LOG_DIR="./exp_log"
NUM_KEYS=1000000
HOT_START=0
HOT_END=199999
VALUE_SIZE=$((16 * 1024))  # 16KB
HOT_RATIO=70

mkdir -p "$LOG_DIR"

# 압축 방식 실험 조합 리스트: "hot_compression cold_compression"
declare -a EXPERIMENTS=(
    "none ZSTD"
    "none Zlib"
    "LZ4 ZSTD"
    "LZ4 Zlib"
    "Snappy ZSTD"
    "Snappy Zlib"
)

# 실행 파일
WRITE_EXEC="./rocksdb_benchmark"
READ_EXEC="./rocksdb_read_benchmark"

# 고정된 컴팩션 스타일
HOT_COMPACTION="level"
COLD_COMPACTION="universal"

# 실험 반복 횟수
NUM_RUNS=3

for ((run=1; run<=NUM_RUNS; run++)); do
    echo "실험 반복 $run 시작"

    for EXP in "${EXPERIMENTS[@]}"; do
        read -r HOT_COMPRESSION COLD_COMPRESSION <<< "$EXP"

        # 로그 파일 지정 (각 반복에 순번 추가)
        WRITE_LOG="$LOG_DIR/write_hot_${HOT_COMPRESSION}_cold_${COLD_COMPRESSION}_run${run}.log"
        READ_LOG="$LOG_DIR/read_hot_${HOT_COMPRESSION}_cold_${COLD_COMPRESSION}_run${run}.log"

        echo "쓰기 실험 시작: hot_compression=$HOT_COMPRESSION, cold_compression=$COLD_COMPRESSION (반복 $run)"
        echo "→ 로그: $WRITE_LOG"

        rm -rf "$DB_PATH"

        "$WRITE_EXEC" "$DB_PATH" "$NUM_KEYS" "$HOT_START" "$HOT_END" "$VALUE_SIZE" "$HOT_RATIO" \
            "$COLD_COMPACTION" "$HOT_COMPACTION" "$COLD_COMPRESSION" "$HOT_COMPRESSION" \
            > "$WRITE_LOG" 2>&1

        echo "읽기 실험 시작: hot_compression=$HOT_COMPRESSION, cold_compression=$COLD_COMPRESSION (반복 $run)"
        echo "→ 로그: $READ_LOG"

        "$READ_EXEC" "$DB_PATH" "$NUM_KEYS" "$HOT_START" "$HOT_END" "$VALUE_SIZE" "$HOT_RATIO" \
            "$COLD_COMPACTION" "$HOT_COMPACTION" "$COLD_COMPRESSION" "$HOT_COMPRESSION" \
            > "$READ_LOG" 2>&1

        echo "완료됨: hot_compression=$HOT_COMPRESSION, cold_compression=$COLD_COMPRESSION (반복 $run)"
        echo "-------------------------------"
    done

    echo "실험 반복 $run 완료 ✅"
    echo "================================="
done

echo "모든 실험 완료 ✅"
