#!/bin/bash

DB_BASE_PATH="./mydb"
VALUE_SIZE=16384           # 16KB
NUM_KEYS=1000000
REPEATS=3
ALPHAS=(0.5 0.9 1.2)

mkdir -p logs

for alpha in "${ALPHAS[@]}"; do
    for run in $(seq 1 $REPEATS); do
        DB_PATH="${DB_BASE_PATH}_alpha${alpha}_run${run}"
        LOG_FILE="logs/zipf_alpha${alpha}_run${run}.log"

        echo "🚀 Running: alpha=${alpha}, run=${run}"
        ./test "$DB_PATH" "$NUM_KEYS" "$VALUE_SIZE" "$alpha" > "$LOG_FILE" 2>&1

        echo "✅ Finished: log saved to $LOG_FILE"
    done
done

echo "🎉 All experiments completed."
