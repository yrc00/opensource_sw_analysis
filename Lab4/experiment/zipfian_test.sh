#!/bin/bash

# 실행 파일 경로 및 DB 경로
EXEC=./zipfdb
DB_PATH=./mydb
NUM_KEYS=1000000
VALUE_SIZE=16384

# 파라미터 배열
alphas=(0.5 0.9 1.2)
precludes_05=(0 20 50 80 120)
precludes_09=(0 15 40 65 120)
precludes_12=(0 10 25 40 120)
temps=(cold)

mkdir -p exp_log

run_test() {
    local run=$1
    local alpha=$2
    local preclude=$3
    local preserve=$4
    local temp=$5

    echo "Running test: run=$run, alpha=$alpha, preclude=$preclude, preserve=$preserve, temp=$temp"
    LOGFILE="exp_log/run${run}_alpha${alpha}_preclude${preclude}_preserve${preserve}_temp${temp}.log"

    # 이전 DB 삭제
    rm -rf "$DB_PATH"

    # 실행 및 로그 저장
    $EXEC "$DB_PATH" "$NUM_KEYS" "$VALUE_SIZE" $alpha $preclude $preserve $temp > "$LOGFILE" 2>&1
}

for run in {1..3}; do
    for alpha in "${alphas[@]}"; do
        if [[ $alpha == 0.5 ]]; then
            precludes=("${precludes_05[@]}")
        elif [[ $alpha == 0.9 ]]; then
            precludes=("${precludes_09[@]}")
        else
            precludes=("${precludes_12[@]}")
        fi

        for temp in "${temps[@]}"; do
            for preclude in "${precludes[@]}"; do
                preserve=$preclude
                run_test $run $alpha $preclude $preserve $temp
            done
        done
    done
done

echo "All tests completed."
