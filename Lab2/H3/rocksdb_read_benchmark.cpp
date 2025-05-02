// rocksdb_read_benchmark.cpp
// 이 코드를 쓰기 전에 먼저 같은 설정으로 데이터를 rocksdb_benchmark.cpp (쓰기용)으로 미리 생성해 두어야 함
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/utilities/options_util.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/statistics.h>
#include <cassert>
#include <chrono>

bool isHotKey(const std::string& key, int hot_start, int hot_end) {
    int key_int = std::stoi(key);
    return key_int >= hot_start && key_int <= hot_end;
}

rocksdb::CompactionStyle parseCompactionStyle(const std::string& style_str) {
    if (style_str == "level") return rocksdb::kCompactionStyleLevel;
    if (style_str == "universal") return rocksdb::kCompactionStyleUniversal;
    if (style_str == "fifo") return rocksdb::kCompactionStyleFIFO;
    if (style_str == "none") return rocksdb::kCompactionStyleNone;

    std::cerr << "지원하지 않는 컴팩션 스타일입니다: " << style_str << std::endl;
    exit(1);
}

int main(int argc, char** argv) {
    if (argc < 9) {
        std::cerr << "사용법: " << argv[0]
                  << " <DB 경로> <총 키 수> <핫 범위 시작> <핫 범위 끝> <value 크기> <핫 접근 비율(0~100)> <default compaction> <hot compaction>"
                  << std::endl;
        return 1;
    }

    std::string db_path = argv[1];
    int num_keys = std::stoi(argv[2]);
    int hot_start = std::stoi(argv[3]);
    int hot_end = std::stoi(argv[4]);
    int value_size = std::stoi(argv[5]); // 사용은 안 해도 통일성 위해 유지
    int hot_ratio = std::stoi(argv[6]);
    std::string default_compaction_str = argv[7];
    std::string hot_compaction_str = argv[8];

    rocksdb::Options options;
    options.create_if_missing = false;
    options.create_missing_column_families = true;

    std::shared_ptr<rocksdb::Statistics> statistics = rocksdb::CreateDBStatistics();
    options.statistics = statistics;

    rocksdb::ColumnFamilyOptions default_cf_options;
    default_cf_options.compaction_style = parseCompactionStyle(default_compaction_str);

    rocksdb::ColumnFamilyOptions hot_cf_options;
    hot_cf_options.compaction_style = parseCompactionStyle(hot_compaction_str);

    std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors = {
        rocksdb::ColumnFamilyDescriptor("default", default_cf_options),
        rocksdb::ColumnFamilyDescriptor("hot", hot_cf_options)
    };

    std::vector<rocksdb::ColumnFamilyHandle*> handles;
    rocksdb::DB* db;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, cf_descriptors, &handles, &db);
    assert(status.ok());

    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<int> key_dist(0, num_keys - 1);
    std::uniform_int_distribution<int> hot_access_dist(0, 99);

    auto start = std::chrono::high_resolution_clock::now();

    int found_hot = 0, found_default = 0;

    for (int i = 0; i < num_keys; ++i) {
        int key = key_dist(rng);
        std::string key_str = std::to_string(key);
        std::string value;

        bool simulate_hot = (hot_access_dist(rng) < hot_ratio);
        bool target_hot = isHotKey(key_str, hot_start, hot_end) && simulate_hot;

        rocksdb::ColumnFamilyHandle* handle = target_hot ? handles[1] : handles[0];

        rocksdb::Status s = db->Get(rocksdb::ReadOptions(), handle, key_str, &value);
        if (s.ok()) {
            target_hot ? found_hot++ : found_default++;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double duration_sec = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;

    std::cout << "읽기 벤치마크 완료!" << std::endl;
    std::cout << "총 소요시간: " << duration_sec << "초" << std::endl;
    std::cout << "hot 컬럼에서 찾은 키 수: " << found_hot << std::endl;
    std::cout << "default 컬럼에서 찾은 키 수: " << found_default << std::endl;

    std::string stats = statistics->ToString();
    std::cout << "RocksDB 통계:\n" << stats << std::endl;

    for (auto* h : handles) db->DestroyColumnFamilyHandle(h);
    delete db;

    return 0;
}