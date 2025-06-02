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

// 문자열을 RocksDB CompactionStyle enum으로 변환하는 함수
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
    // 인자 수 검사
    if (argc < 9) {
        std::cerr << "사용법: " << argv[0] 
                  << " <DB 경로> <총 키 수> <핫 범위 시작> <핫 범위 끝> <value 크기> <핫 접근 비율(0~100)> <default 컬럼 compaction> <hot 컬럼 compaction>"
                  << std::endl;
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

    // RocksDB 옵션 설정
    rocksdb::Options options;
    options.create_if_missing = true;
    options.create_missing_column_families = true;

    // 통계 수집을 위한 Statistics 객체 생성
    std::shared_ptr<rocksdb::Statistics> statistics = rocksdb::CreateDBStatistics();
    options.statistics = statistics;

    // 각 컬럼 패밀리의 compaction style 지정
    rocksdb::ColumnFamilyOptions default_cf_options;
    default_cf_options.compaction_style = parseCompactionStyle(default_compaction_str);

    rocksdb::ColumnFamilyOptions hot_cf_options;
    hot_cf_options.compaction_style = parseCompactionStyle(hot_compaction_str);

    // 컬럼 패밀리 정의
    // Column Family에 대한 Descriptor 생성
    std::vector<std::string> cf_names = {"default", "hot"};
    std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors;
    cf_descriptors.emplace_back("default", default_cf_options);
    cf_descriptors.emplace_back("hot", hot_cf_options);

    // 지정된 Column Family와 함께 RocksDB 오픈
    std::vector<rocksdb::ColumnFamilyHandle*> handles;
    rocksdb::DB* db;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, cf_descriptors, &handles, &db);
    assert(status.ok());

    // 난수 생성기 설정
    std::default_random_engine rng(std::random_device{}());
    std::uniform_int_distribution<int> hot_key_dist(hot_start, hot_end);     // hot 영역에서 무작위 키를 생성하기 위한 분포
    std::uniform_int_distribution<int> any_key_dist(0, num_keys - 1);       // 전체 key 영역에서 무작위 키를 생성하기 위한 분포 (콜드 키 선택용)
    std::uniform_int_distribution<int> hot_access_dist(0, 99);              // hot_ratio 퍼센트 비율로 hot access 여부 결정

    // cold key는 hot 범위를 제외하고 직접 무작위 생성
    auto generate_cold_key = [&](std::default_random_engine& rng) {
        while (true) {
            int k = any_key_dist(rng);  // 전체 범위에서 무작위로 뽑음
            if (k < hot_start || k > hot_end) return k;  // hot 영역이 아니면 cold로 간주하고 반환
    }
        }
    };

    // 시간 측정 시작
    auto start = std::chrono::high_resolution_clock::now();

    // 키를 생성하여 hot 또는 cold 컬럼 패밀리에 저장
    for (int i = 0; i < num_keys; ++i) {
		    // 설정한 확률대로 key 결정
        bool is_hot_access = (hot_access_dist(rng) < hot_ratio);

				// 키 값 결정: hot이면 hot_key_dist로, cold면 generate_cold_key로 생성
        int key = is_hot_access ? hot_key_dist(rng) : generate_cold_key(rng);
        std::string key_str = std::to_string(key);
        std::string value(value_size, 'v');  // value_size 만큼 v로 채운 문자열
				// hot, cold에 따라 컬럼 패밀리 핸들링
        rocksdb::ColumnFamilyHandle* target_handle = is_hot_access ? handles[1] : handles[0];
				
				// 데이터 넣기
        db->Put(rocksdb::WriteOptions(), target_handle, key_str, value);
    }

    // 시간 측정 종료
    auto end = std::chrono::high_resolution_clock::now();
    double duration_sec = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;

    std::cout << "워크로드 생성 완료!" << std::endl;
    std::cout << "총 소요시간: " << duration_sec << "초\n" << std::endl;

    // 각 컬럼 패밀리에 실제로 저장된 키 개수 확인
    uint64_t hot_count = 0;
    uint64_t default_count = 0;

    rocksdb::ReadOptions read_options;
    for (int i = 0; i < num_keys; ++i) {
        std::string key_str = std::to_string(i);
        std::string value;

        // hot 컬럼 먼저 조회
        rocksdb::Status hot_status = db->Get(read_options, handles[1], key_str, &value);
        if (hot_status.ok()) {
            hot_count++;
            continue;
        }

        // hot에 없으면 default에서 조회
        rocksdb::Status default_status = db->Get(read_options, handles[0], key_str, &value);
        if (default_status.ok()) {
            default_count++;
        }
    }

    std::cout << "hot 컬럼에 저장된 키 수: " << hot_count << std::endl;
    std::cout << "default 컬럼에 저장된 키 수: " << default_count << std::endl;

    // RocksDB 내부 통계 출력
    std::string stats = statistics->ToString();
    std::cout << "RocksDB 통계:\n" << stats << std::endl;

    // 핸들 정리 및 DB 종료
    for (auto* h : handles) {
        db->DestroyColumnFamilyHandle(h);
    }

    delete db;
    return 0;
}
