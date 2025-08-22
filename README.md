<img alt="RocksDB" src="https://img.shields.io/badge/RocksDB-2A2A2A.svg?style=for-the-badge&logo=RocksDB&logoColor=white" height="20"/> <img alt="C++" src="https://img.shields.io/badge/C++-00599C.svg?style=for-the-badge&logo=C++&logoColor=white" height="20"/> <img alt="Python" src="https://img.shields.io/badge/Python-3776AB.svg?style=for-the-badge&logo=Python&logoColor=white" height="20"/>

</br>

# 💻 opensource_sw_analysis

2025년 1학기 **오픈소스분석(빅데이터)** 수업의 과제를 위한 레포지토리입니다 RocksDB를 활용한 실험 코드, 로그 데이터 그리고 분석 코드를 공유합니다. 

각 실험에 대한 자세한 내용은 Lab 폴더별로 ppt가 첨부되어 있습니다.

</br>

## 🔎 Lab1
**가설 1**
> WAL 설정과 Compaction 전략 간의 상호작용은 WAF에 유의미한 영향을 미칠 수 있다.
- 유의미한 영향을 준 것으로 확인

</br>

**가설 2**
> Update 및 Delete가 빈번한 워크로드에서 Leveled Compaction의 Compaction 빈도를 최적화할 경우, Universal compaction과 비슷한 수준의 WAF 성능을 달성할 수 있다. 
- `max_bytes_for_level_base`, `max_bytes_for_multiplier`가 작을수록 WAF ↓

</br>

## 🔎 Lab2
**가설1 추가실험**
> WAL 설정과 Compaction 전략 간의 상호작용은 WAF에 유의미한 영향을 미칠 수 있다.
- WAF 측정 시 Direct-IO가 영향을 미칠 가능성이 있어 이와 관련된 추가 실험 진행
- WAL 설정보다 Direct-IO의 WAF 영향이 더 큼 → Direct-IO의 영향 확인

</br>

**가설 3**
> Column Family 별로 Hot, Cold Data를 저장하고, Universal Compaction과 Leveld Compaction을 나누어 사용하면 DB 성능이 좋아질 것이다. 
- Read/Write 성능 균형 향상, 단일 전략 대비 우수

</br>

## 🔎 Lab3
**가설 4**
> Column Family별로 Hot, Cold Data를 저장하고, 각각 다른 압축 알고리즘을 적용하면 DB 성능이 더 좋아질 것이다. 
- 쓰기엔 ZSTD, 읽기엔 LZ4를 사용하는 조합이 대부분 효과적

</br>

## 🔎 Lab4
**가설 5**
> Universal Compaction만 사용한 경우와 비교하여 Time-Aware Tiered Storage (TATS)는 오버헤드가 발생하여 쓰기 성능이 더 낮게 나올 것이다
- 유의미한 성능 저하 없음

</br>

**가설 6**
> Cold 분류 기준에 따라 Time-Aware Tiered Storage의 효과가 달라질 수 있으며, 특히 preclude_last_level_data_seconds의 값이 커지면 WAF의 감소로 쓰기 성능이 향상될 것이다. 
- WAF 변화는 미미, 그러나 쓰기 성능은 일부 차이 발생
