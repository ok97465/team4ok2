#pragma hdrstop
#include <cmath>
#include <chrono>
#include <algorithm>
#include "ProximityAssessor.h"
#include "SpatialGrid.h"
#include "CPA.h"
#include "LogHandler.h"

// TAssessmentThread 생성자 (변경 없음)
__fastcall TAssessmentThread::TAssessmentThread(const std::vector<TADS_B_Aircraft*>& snapshot, double hThresh, double vThresh, int timeSec, double minTimeSec)
    : TThread(false), FAircraftSnapshot(snapshot), FHorizontalThreshold(hThresh), FVerticalThreshold(vThresh), FTimeSec(timeSec), FMinTimeSec(minTimeSec)
{
    FreeOnTerminate = true;
}

// TAssessmentThread 실행 함수 수정
void __fastcall TAssessmentThread::Execute()
{
    auto start = std::chrono::steady_clock::now();
    
    SpatialGrid* grid = new SpatialGrid();

    for (auto ac : FAircraftSnapshot) {
        grid->add(ac);
    }

    std::vector<ConflictPair> localConflictList;

    // 근접 평가 수행
    for (const auto& aircraft1 : FAircraftSnapshot) {
        if (Terminated) break;
        if (aircraft1->Altitude <= 0) continue;
        std::vector<TADS_B_Aircraft*> candidates = grid->getNearbyAircraft(aircraft1);

        for (const auto& aircraft2 : candidates) {
            if (aircraft1->ICAO >= aircraft2->ICAO) continue;
            if (aircraft2->Altitude <= 0) continue;
            if (!aircraft1->HaveSpeedAndHeading || !aircraft2->HaveSpeedAndHeading) continue;
            if (std::abs(aircraft1->Altitude - aircraft2->Altitude) > FVerticalThreshold) continue;

            double tcpa, cpa_distance_nm, vertical_cpa;
            if (computeCPA(aircraft1->Latitude, aircraft1->Longitude, aircraft1->Altitude,
                           aircraft1->Speed, aircraft1->Heading,
                           aircraft2->Latitude, aircraft2->Longitude, aircraft2->Altitude,
                           aircraft2->Speed, aircraft2->Heading,
                           tcpa, cpa_distance_nm, vertical_cpa))
            {
                // [수정] 새로운 충돌 조건 확인
                // 1. 미래에 최단 근접 지점이 있고 (tcpa > minTCPA)
                // 2. 그 시간이 너무 멀지 않으며 (maxTCPA)
                // 3. 최단 근접 거리가 임계값보다 작은 경우
                if (tcpa > FMinTimeSec && tcpa < FTimeSec && cpa_distance_nm < FHorizontalThreshold) {
                    localConflictList.push_back({aircraft1->ICAO, aircraft2->ICAO, cpa_distance_nm, tcpa, vertical_cpa});
                }
            }
        }
    }

    delete grid;

    // 가중치 정의
    const double H_DIST_THRESHOLD = FHorizontalThreshold;
    const double V_DIST_THRESHOLD = FVerticalThreshold;
    const double TIME_THRESHOLD = static_cast<double>(FTimeSec);
    const double MIN_TIME_THRESHOLD = FMinTimeSec;
    const double w_time = 3.0, w_vert = 2.0, w_horiz = 1.0;
    const double MAX_POSSIBLE_SCORE = w_time + w_vert + w_horiz;

    // 정규화 기반 위험 점수 계산 람다 함수
    auto calculateThreatScore = [&](double tcpa, double h_dist, double v_dist) {
        tcpa = std::max(MIN_TIME_THRESHOLD, std::min(tcpa, TIME_THRESHOLD));
        h_dist = std::max(0.0, std::min(h_dist, H_DIST_THRESHOLD));
        v_dist = std::max(0.0, std::min(v_dist, V_DIST_THRESHOLD));

        // 1. "위험 근접도" 계산 (0.0 ~ 1.0)
        // TCPA 근접도는 Min~Max 범위 내에서 계산
        double time_range = TIME_THRESHOLD - MIN_TIME_THRESHOLD;
        double time_proximity = 1.0 - ((tcpa - MIN_TIME_THRESHOLD) / time_range);
        double horiz_proximity = 1.0 - (h_dist / H_DIST_THRESHOLD);
        double vert_proximity = 1.0 - (v_dist / V_DIST_THRESHOLD);

        // 2. 위험도를 제곱하여 가중치 적용
        double raw_score = (w_time * time_proximity * time_proximity) +
                           (w_horiz * horiz_proximity * horiz_proximity) +
                           (w_vert * vert_proximity * vert_proximity);

        // 3. 최종 점수 반환
        return (raw_score / MAX_POSSIBLE_SCORE) * 10.0;
    };

    // 4-1. 각 충돌 후보 쌍에 대해 위험 점수를 계산하여 저장합니다.
    for (auto& conflict : localConflictList) {
        conflict.threatScore = calculateThreatScore(conflict.timeToCPA, conflict.cpaDistance, conflict.verticalCPA);
    }

    std::sort(localConflictList.begin(), localConflictList.end(),
        [](const ConflictPair& a, const ConflictPair& b) {
            return a.threatScore > b.threatScore;
    });

    if (!Terminated) {
        // 1. 정렬된 리스트 저장
        this->SortedConflictList = localConflictList;

        // 2. 맵 생성 및 저장
        this->ConflictMap.clear();
        for (const auto& pair : this->SortedConflictList) {
            this->ConflictMap[pair.ICAO1].push_back({pair.ICAO2, pair.cpaDistance, pair.timeToCPA, pair.verticalCPA});
            this->ConflictMap[pair.ICAO2].push_back({pair.ICAO1, pair.cpaDistance, pair.timeToCPA, pair.verticalCPA});
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    LOG_DEBUG_F(LogHandler::CAT_PERFORMANCE, "Proximity assessment execution time: %lld ms", duration);
}

// ProximityAssessor 클래스 구현 (변경 없음)
ProximityAssessor::ProximityAssessor() : TObject(), FWorkerThread(NULL), FOnComplete(NULL) {}
ProximityAssessor::~ProximityAssessor() { if (FWorkerThread) FWorkerThread->Terminate(); }

void ProximityAssessor::startAssessment(const std::vector<TADS_B_Aircraft*>& aircraftSnapshot, double hThresh, double vThresh, int timeSec, double minTimeSec)
{
    if (FWorkerThread) return;
    FWorkerThread = new TAssessmentThread(aircraftSnapshot, hThresh, vThresh, timeSec, minTimeSec);
    FWorkerThread->OnTerminate = ThreadTerminated;
}

void __fastcall ProximityAssessor::ThreadTerminated(TObject* Sender)
{
    if (FWorkerThread) {
        // 스레드가 만든 두 결과물을 모두 복사
        SortedListResults = FWorkerThread->SortedConflictList;
        MapResults = FWorkerThread->ConflictMap;
    }
    FWorkerThread = NULL;
    if (FOnComplete) FOnComplete(this);
}