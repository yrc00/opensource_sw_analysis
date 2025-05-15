// 필수 헤더 포함 - 쓰기 수정 버전
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

// 문자열을 RocksDB CompactionStyle enum으로 변환
rocksdb::CompactionStyle parseCompactionStyle(const std::string& style_str) {
    if (style_str == "level") return rocksdb::kCompactionStyleLevel;
    if (style_str == "universal") return rocksdb::kCompactionStyleUniversal;
    if (style_str == "fifo") return rocksdb::kCompactionStyleFIFO;
    if (style_str == "none") return rocksdb::kCompactionStyleNone;
    std::cerr << "지원하지 않는 compaction 스타일: " << style_str << std::endl;
    exit(1);
}

// 문자열을 RocksDB CompressionType enum으로 변환
rocksdb::CompressionType parseCompressionType(const std::string& comp_str) {
    if (comp_str == "none") return rocksdb::kNoCompression;
    if (comp_str == "Snappy") return rocksdb::kSnappyCompression;
    if (comp_str == "Zlib") return rocksdb::kZlibCompression;
    if (comp_str == "BZip2") return rocksdb::kBZip2Compression;
    if (comp_str == "LZ4") return rocksdb::kLZ4Compression;
    if (comp_str == "ZSTD") return rocksdb::kZSTD;
    std::cerr << "지원하지 않는 압축 방식: " << comp_str << std::endl;
    exit(1);
}

int main(int argc, char** argv) {
    if (argc < 11) {
        std::cerr << "사용법: " << argv[0]
                  << " <DB 경로> <총 키 수> <핫 범위 시작> <핫 범위 끝> <value 크기> <핫 접근 비율(0~100)>"
                  << " <default compaction> <hot compaction> <default compression> <hot compression>\n";
        return 1;
    }

    // 인자 파싱
    std::string db_path = argv[1];
    int num_keys = std::stoi(argv[2]);
    int hot_start = std::stoi(argv[3]);
    int hot_end = std::stoi(argv[4]);
    int value_size = std::stoi(argv[5]);
    int hot_ratio = std::stoi(argv[6]);
    std::string default_compaction_str = argv[7];
    std::string hot_compaction_str = argv[8];
    std::string default_compression_str = argv[9];
    std::string hot_compression_str = argv[10];

    // DB 옵션 설정
    rocksdb::Options options;
    options.create_if_missing = true;
    options.create_missing_column_families = true;

    std::shared_ptr<rocksdb::Statistics> statistics = rocksdb::CreateDBStatistics();
    options.statistics = statistics;

    // Column Family별 옵션 설정
    rocksdb::ColumnFamilyOptions default_cf_options;
    default_cf_options.compaction_style = parseCompactionStyle(default_compaction_str);
    default_cf_options.compression = parseCompressionType(default_compression_str);

    rocksdb::ColumnFamilyOptions hot_cf_options;
    hot_cf_options.compaction_style = parseCompactionStyle(hot_compaction_str);
    hot_cf_options.compression = parseCompressionType(hot_compression_str);

    // CF Descriptor 생성
    std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors = {
        rocksdb::ColumnFamilyDescriptor("default", default_cf_options),
        rocksdb::ColumnFamilyDescriptor("hot", hot_cf_options)
    };

    // DB 오픈
    rocksdb::DB* db;
    std::vector<rocksdb::ColumnFamilyHandle*> handles;
    auto status = rocksdb::DB::Open(options, db_path, cf_descriptors, &handles, &db);
    assert(status.ok());

    // 랜덤 관련 초기화
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<int> hot_key_dist(hot_start, hot_end);
    std::uniform_int_distribution<int> any_key_dist(0, num_keys - 1);
    std::uniform_int_distribution<int> hot_access_dist(0, 99);

    auto generate_cold_key = [&](std::default_random_engine& rng) {
        while (true) {
            int k = any_key_dist(rng);
            if (k < hot_start || k > hot_end) return k;
        }
    };

    auto start = std::chrono::high_resolution_clock::now();

    // 데이터 삽입
    for (int i = 0; i < num_keys; ++i) {
        bool is_hot = (hot_access_dist(rng) < hot_ratio);
        int key = is_hot ? hot_key_dist(rng) : generate_cold_key(rng);
        std::string key_str = std::to_string(key);
        std::string value(value_size, 'v');
        auto* handle = is_hot ? handles[1] : handles[0];
        db->Put(rocksdb::WriteOptions(), handle, key_str, value);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;

    std::cout << "워크로드 생성 완료!" << std::endl;
    std::cout << "총 소요시간: " << duration << "초\n";

    // 저장된 키 개수 카운팅
    uint64_t hot_count = 0, default_count = 0;
    rocksdb::ReadOptions read_opts;

    for (int i = 0; i < num_keys; ++i) {
        std::string key_str = std::to_string(i);
        std::string value;

        if (db->Get(read_opts, handles[1], key_str, &value).ok()) {
            hot_count++;
        } else if (db->Get(read_opts, handles[0], key_str, &value).ok()) {
            default_count++;
        }
    }

    std::cout << "hot 컬럼에 저장된 키 수: " << hot_count << std::endl;
    std::cout << "default 컬럼에 저장된 키 수: " << default_count << std::endl;

    // 통계 출력
    std::cout << "RocksDB 통계:\n" << statistics->ToString() << std::endl;

    for (auto* h : handles) db->DestroyColumnFamilyHandle(h);
    delete db;
    return 0;
}
