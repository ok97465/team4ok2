// -- Model/AircraftDataModel.h --

#ifndef AircraftDataModelH
#define AircraftDataModelH

#include <System.Classes.hpp>
#include "ght_hash_table.h"
#include "Aircraft.h"

class AircraftDataModel : public TObject
{
private:
    ght_hash_table_t* FHashTable;
    int FCurrentSpriteImage;

    // 내부에서만 사용될 헬퍼 함수
    TADS_B_Aircraft* FindOrCreateAircraft(unsigned int Addr, bool ACycleImages, int ANumSpriteImages);

public:
    __fastcall AircraftDataModel();
    __fastcall ~AircraftDataModel();

    // --- 데이터 처리 인터페이스 ---
    void ProcessRawMessage(const String& data, bool ACycleImages, int ANumSpriteImages);
    void ProcessSbsMessage(const String& data);

    // --- 데이터 관리 인터페이스 ---
    void PurgeStaleAircraft(int AStaleTimeSec);
    void PurgeAllAircraft();
    int GetAircraftCount();

    // --- 데이터 조회/반복 인터페이스 (UI 렌더링용) ---
    TADS_B_Aircraft* GetFirstAircraft(ght_iterator_t* iterator, const void** key);
    TADS_B_Aircraft* GetNextAircraft(ght_iterator_t* iterator, const void** key);
    TADS_B_Aircraft* FindAircraftByICAO(unsigned int Addr);
};

#endif