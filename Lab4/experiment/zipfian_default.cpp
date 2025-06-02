#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/env.h>
#include <rocksdb/statistics.h>
#include <rocksdb/iostats_context.h>
#include <rocksdb/metadata.h>
#include <rocksdb/utilities/options_util.h>
#include <rocksdb/utilities/db_ttl.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <map>
#include <vector>
#include <cmath>

using namespace rocksdb;

// ⏱️ 실행 시간 측정
auto StartTimer() {
    return std::chrono::high_resolution_clock::now();
}

void StopTimer(std::chrono::high_resolution_clock::time_point start) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\n⏱️ 실행 시간: " << duration.count() << " ms\n";
}

// 📊 SST 파일 온도 상태 출력
void PrintSstFileTemperature(DB* db) {
    std::vector<LiveFileMetaData> metadata_list;
    db->GetLiveFilesMetaData(&metadata_list);

    std::map<int, std::map<Temperature, int>> level_temp_counts;
    for (const auto& meta : metadata_list) {
        level_temp_counts[meta.level][meta.temperature]++;
    }

    std::cout << "\n===== Level별 SST 파일 개수 및 온도별 분포 =====\n";
    for (const auto& [level, temp_map] : level_temp_counts) {
        int total = 0;
        for (const auto& [temp, count] : temp_map) total += count;
        std::cout << "Level " << level << " : 총 " << total << "개 파일\n";
        for (const auto& [temp, count] : temp_map) {
            std::string temp_str = "Unknown";
            if (temp == Temperature::kHot) temp_str = "Hot";
            else if (temp == Temperature::kWarm) temp_str = "Warm";
            else if (temp == Temperature::kCold) temp_str = "Cold";
            std::cout << "  " << temp_str << " : " << count << "개\n";
        }
    }
    std::cout << "===============================================\n";
}

uint64_t get_directory_size(const std::string& path) {
    std::string cmd = "du -sb " + path + " 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return 0;
    uint64_t size = 0;
    fscanf(pipe, "%lu", &size);
    pclose(pipe);
    return size;
}

// 🎯 시드 고정 ZipfGenerator 클래스
class ZipfGenerator {
    std::default_random_engine generator;
    std::discrete_distribution<int> distribution;

public:
    ZipfGenerator(int n, double alpha)
        : generator(42)  // 시드 고정
    {
        std::vector<double> weights(n);
        for (int i = 0; i < n; ++i) {
            weights[i] = 1.0 / pow(i + 1, alpha);
        }
        distribution = std::discrete_distribution<int>(weights.begin(), weights.end());
    }

    int next() {
        return distribution(generator);
    }
};

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Usage: ./zipfdb <db_path> <num_keys> <value_size> <zipf_alpha>\n";
        return 1;
    }

    auto start_time = StartTimer();

    std::string db_path = argv[1];
    int num_keys = std::stoi(argv[2]);
    int value_size = std::stoi(argv[3]);
    double alpha = std::stod(argv[4]);

    // ✅ RocksDB 기본 옵션 설정
    Options options;
    options.create_if_missing = true;
    options.statistics = CreateDBStatistics();
    auto stats = options.statistics;
    options.compaction_style = kCompactionStyleUniversal;

    // 🧹 DB 제거 후 시작
    DestroyDB(db_path, options);

    DB* db;
    Status s = DB::Open(options, db_path, &db);
    if (!s.ok()) {
        std::cerr << "Failed to open DB: " << s.ToString() << "\n";
        return 1;
    }

    std::string value(value_size, 'Z');

    ZipfGenerator zipf(num_keys, alpha);  // ZipfGenerator 생성

    auto start = std::chrono::high_resolution_clock::now();  // 쓰기 시작 시간

    std::cout << "[💾 Writing " << num_keys << " keys...]\n";
    for (int i = 0; i < num_keys; ++i) {
        int key_id = zipf.next();  // Zipfian key 생성
        std::string key = "key_" + std::to_string(key_id);
        db->Put(WriteOptions(), key, value);

        if (i > 0 && i % 100000 == 0) {
            std::cout << "Inserted " << i << " keys\n";
        }
    }

    auto end = std::chrono::high_resolution_clock::now();  // 쓰기 종료 시간
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "\n⏱️ Total write time: " << elapsed.count() << " seconds\n";

    uint64_t db_size = get_directory_size(db_path);
    std::cout << "DB 디렉토리 사용량: " << db_size << " bytes (" << db_size / (1024.0 * 1024.0) << " MB)" << std::endl;

    std::cout << "\n📊 RocksDB Statistics:\n";
    std::cout << db->GetOptions().statistics->ToString() << std::endl;

    std::cout << "\n[📊 Temperature Distribution of SST Files]\n";
    PrintSstFileTemperature(db);

    delete db;
    std::cout << "\n✅ Done.\n";
    return 0;
}
