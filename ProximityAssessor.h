#ifndef ProximityAssessorH
#define ProximityAssessorH

#include <vector>
#include <map>
#include <System.Classes.hpp>
#include "Aircraft.h"

struct ConflictPair {
    unsigned int ICAO1;
    unsigned int ICAO2;
    double cpaDistance;
    double timeToCPA;
    double verticalCPA;
    double threatScore;
};

struct RelatedConflictInfo {
    unsigned int otherAircraftICAO;
    double cpaDistance;
    double timeToCPA;
    double verticalCPA;
};

class TAssessmentThread : public TThread {
private:
    std::vector<TADS_B_Aircraft*> FAircraftSnapshot;
    double FHorizontalThreshold;
    double FVerticalThreshold;
    int FTimeSec;
    double FMinTimeSec;  // TCPA Min 값 추가
protected:
    void __fastcall Execute() override;
public:
    __fastcall TAssessmentThread(const std::vector<TADS_B_Aircraft*>& snapshot, double hThresh, double vThresh, int timeSec, double minTimeSec);
    //__fastcall TAssessmentThread();
    std::vector<ConflictPair> SortedConflictList;
    std::map<unsigned int, std::vector<RelatedConflictInfo>> ConflictMap;
};

// [수정] TObject를 상속하도록 변경
class ProximityAssessor : public TObject
{
private:
    TAssessmentThread* FWorkerThread;
    TNotifyEvent FOnComplete;
    void __fastcall ThreadTerminated(TObject* Sender);
public:
    ProximityAssessor(); // 생성자 수정
    ~ProximityAssessor();
    void startAssessment(const std::vector<TADS_B_Aircraft*>& aircraftSnapshot, double hThresh, double vThresh, int timeSec, double minTimeSec);
    __property TNotifyEvent OnComplete = {read=FOnComplete, write=FOnComplete};
    std::vector<ConflictPair> SortedListResults;
    std::map<unsigned int, std::vector<RelatedConflictInfo>> MapResults;
};
#endif