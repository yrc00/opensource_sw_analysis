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

rocksdb::CompactionStyle parseCompactionStyle(const std::string& style_str) {
    if (style_str == "level") return rocksdb::kCompactionStyleLevel;
    if (style_str == "universal") return rocksdb::kCompactionStyleUniversal;
    if (style_str == "fifo") return rocksdb::kCompactionStyleFIFO;
    if (style_str == "none") return rocksdb::kCompactionStyleNone;

    std::cerr << "지원하지 않는 컴팩션 스타일입니다: " << style_str << std::endl;
    exit(1);
}

rocksdb::CompressionType parseCompressionType(const std::string& comp_str) {
    if (comp_str == "none") return rocksdb::kNoCompression;
    if (comp_str == "Snappy") return rocksdb::kSnappyCompression;
    if (comp_str == "Zlib") return rocksdb::kZlibCompression;
    if (comp_str == "BZip2") return rocksdb::kBZip2Compression;
    if (comp_str == "LZ4") return rocksdb::kLZ4Compression;
    if (comp_str == "ZSTD") return rocksdb::kZSTD;

    std::cerr << "지원하지 않는 압축 타입입니다: " << comp_str << std::endl;
    exit(1);
}

int generate_cold_key(std::default_random_engine& rng, int hot_start, int hot_end, int num_keys) {
    std::uniform_int_distribution<int> dist(0, num_keys - 1);
    int key;
    do {
        key = dist(rng);
    } while (key >= hot_start && key <= hot_end); // hot 범위 피하기
    return key;
}

int main(int argc, char** argv) {
    if (argc < 11) {
        std::cerr << "사용법: " << argv[0]
                  << " <DB 경로> <총 키 수> <핫 범위 시작> <핫 범위 끝> <value 크기> <핫 접근 비율(0~100)>"
                  << " <default compaction> <hot compaction> <default compression> <hot compression>" << std::endl;
        return 1;
    }

    std::string db_path = argv[1];
    int num_keys = std::stoi(argv[2]);
    int hot_start = std::stoi(argv[3]);
    int hot_end = std::stoi(argv[4]);
    int value_size = std::stoi(argv[5]); // unused
    int hot_ratio = std::stoi(argv[6]);
    std::string default_compaction_str = argv[7];
    std::string hot_compaction_str = argv[8];
    std::string default_compression_str = argv[9];
    std::string hot_compression_str = argv[10];

    rocksdb::Options options;
    options.create_if_missing = false;
    options.create_missing_column_families = true;
    options.statistics = rocksdb::CreateDBStatistics();

    rocksdb::ColumnFamilyOptions default_cf_options;
    default_cf_options.compaction_style = parseCompactionStyle(default_compaction_str);
    default_cf_options.compression = parseCompressionType(default_compression_str);

    rocksdb::ColumnFamilyOptions hot_cf_options;
    hot_cf_options.compaction_style = parseCompactionStyle(hot_compaction_str);
    hot_cf_options.compression = parseCompressionType(hot_compression_str);

    std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors = {
        rocksdb::ColumnFamilyDescriptor("default", default_cf_options),
        rocksdb::ColumnFamilyDescriptor("hot", hot_cf_options)
    };

    std::vector<rocksdb::ColumnFamilyHandle*> handles;
    rocksdb::DB* db;
    auto status = rocksdb::DB::Open(options, db_path, cf_descriptors, &handles, &db);
    assert(status.ok());

    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<int> hot_key_dist(hot_start, hot_end);
    std::uniform_int_distribution<int> hot_access_dist(0, 99);

    int found_hot = 0, found_default = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_keys; ++i) {
        bool is_hot_access = (hot_access_dist(rng) < hot_ratio);

        int key = is_hot_access
                    ? hot_key_dist(rng)
                    : generate_cold_key(rng, hot_start, hot_end, num_keys);

        std::string key_str = std::to_string(key);
        std::string value;

        rocksdb::ColumnFamilyHandle* target_handle = is_hot_access ? handles[1] : handles[0];

        rocksdb::Status s = db->Get(rocksdb::ReadOptions(), target_handle, key_str, &value);
        if (s.ok()) {
            is_hot_access ? found_hot++ : found_default++;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double duration_sec = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;

    std::cout << "읽기 벤치마크 완료!" << std::endl;
    std::cout << "총 소요시간: " << duration_sec << "초" << std::endl;
    std::cout << "hot 컬럼에서 찾은 키 수: " << found_hot << std::endl;
    std::cout << "default 컬럼에서 찾은 키 수: " << found_default << std::endl;

    std::cout << "RocksDB 통계:\n" << options.statistics->ToString() << std::endl;

    for (auto* h : handles) db->DestroyColumnFamilyHandle(h);
    delete db;

    return 0;
}
