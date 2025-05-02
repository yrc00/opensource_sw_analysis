// rocksdb_benchmark.cpp
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

// 범위 내 key 인지 확인
bool isHotKey(const std::string& key, int hot_start, int hot_end) {
    int key_int = std::stoi(key);
    return key_int >= hot_start && key_int <= hot_end;
}

// 문자열을 RocksDB CompactionStyle로 변환
rocksdb::CompactionStyle parseCompactionStyle(const std::string& style_str) {
    if (style_str == "level") {
        return rocksdb::kCompactionStyleLevel;
    } else if (style_str == "universal") {
        return rocksdb::kCompactionStyleUniversal;
    } else if (style_str == "fifo") {
        return rocksdb::kCompactionStyleFIFO;
    } else if (style_str == "none") {
        return rocksdb::kCompactionStyleNone;
    } else {
        std::cerr << "지원하지 않는 컴팩션 스타일입니다: " << style_str << std::endl;
        exit(1);
    }
}

int main(int argc, char** argv) {
    if (argc < 9) {
        std::cerr << "사용법: " << argv[0] 
                  << " <DB 경로> <총 키 수> <핫 범위 시작> <핫 범위 끝> <value 크기> <핫 접근 비율(0~100)> <default 컬럼 compaction> <hot 컬럼 compaction>"
                  << std::endl;
        std::cerr << "예시: " << argv[0] << " ./mydb 1000000 0 200000 4096 70 universal level" << std::endl;
        return 1;
    }

    std::string db_path = argv[1];
    int num_keys = std::stoi(argv[2]);
    int hot_start = std::stoi(argv[3]);
    int hot_end = std::stoi(argv[4]);
    int value_size = std::stoi(argv[5]);
    int hot_ratio = std::stoi(argv[6]);
    std::string default_compaction_str = argv[7];
    std::string hot_compaction_str = argv[8];

    // RocksDB 옵션
    rocksdb::Options options;
    options.create_if_missing = true;
    options.create_missing_column_families = true;

    std::shared_ptr<rocksdb::Statistics> statistics = rocksdb::CreateDBStatistics();
    options.statistics = statistics;

    // 명령줄 인자로 받은 compaction style 설정
    rocksdb::ColumnFamilyOptions default_cf_options;
    default_cf_options.compaction_style = parseCompactionStyle(default_compaction_str);

    rocksdb::ColumnFamilyOptions hot_cf_options;
    hot_cf_options.compaction_style = parseCompactionStyle(hot_compaction_str);

    std::vector<std::string> cf_names = {"default", "hot"};
    std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors;
    cf_descriptors.emplace_back("default", default_cf_options);
    cf_descriptors.emplace_back("hot", hot_cf_options);

    std::vector<rocksdb::ColumnFamilyHandle*> handles;
    rocksdb::DB* db;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, cf_descriptors, &handles, &db);
    assert(status.ok());

    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<int> key_dist(0, num_keys - 1);
    std::uniform_int_distribution<int> hot_access_dist(0, 99);

		auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_keys; ++i) {
        int key = key_dist(rng);
        std::string key_str = std::to_string(key);
        std::string value(value_size, 'v');

        bool simulate_hot = (hot_access_dist(rng) < hot_ratio);
        bool target_hot = isHotKey(key_str, hot_start, hot_end) && simulate_hot;

        rocksdb::ColumnFamilyHandle* target_handle = target_hot ? handles[1] : handles[0];

        db->Put(rocksdb::WriteOptions(), target_handle, key_str, value);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double duration_sec = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;
       
    std::cout << "워크로드 생성 완료!" << std::endl;
    std::cout << "총 소요시간: " << duration_sec << "초\n" << std::endl;
        
    // 컬럼 패밀리별 키 개수 직접 세기
    uint64_t hot_count = 0;
    uint64_t default_count = 0;

    rocksdb::ReadOptions read_options;
    for (int i = 0; i < num_keys; ++i) {
        std::string key_str = std::to_string(i);
        std::string value;

        rocksdb::Status hot_status = db->Get(read_options, handles[1], key_str, &value);
        if (hot_status.ok()) {
            hot_count++;
            continue;
        }

        rocksdb::Status default_status = db->Get(read_options, handles[0], key_str, &value);
        if (default_status.ok()) {
            default_count++;
        }
    }

    std::cout << "hot 컬럼에 저장된 키 수: " << hot_count << std::endl;
    std::cout << "default 컬럼에 저장된 키 수: " << default_count << std::endl;

    // 통계 출력
    std::string stats = statistics->ToString();
    std::cout << "RocksDB 통계:\n" << stats << std::endl;

    for (auto* h : handles) {
        db->DestroyColumnFamilyHandle(h);
    }

    delete db;
    return 0;
}