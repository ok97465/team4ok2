// -- Model/AircraftDataModel.cpp --

#pragma hdrstop

#include "AircraftDataModel.h"
#include "DecodeRawADS_B.h"
#include "SBS_Message.h"
#include "TimeFunctions.h"

#pragma link "HashTable\Lib\Win64\Release\HashTableLib.a"

//---------------------------------------------------------------------------
__fastcall AircraftDataModel::AircraftDataModel()
{
    FCurrentSpriteImage = 0;
    FHashTable = ght_create(50000); // 해시테이블 생성
    if (!FHashTable)
    {
        throw Sysutils::Exception("Create Hash Failed");
    }
    ght_set_rehash(FHashTable, TRUE);
}
//---------------------------------------------------------------------------
__fastcall AircraftDataModel::~AircraftDataModel()
{
    PurgeAllAircraft(); // 모든 항공기 객체 메모리 해제
    ght_finalize(FHashTable); // 해시테이블 메모리 해제
}
//---------------------------------------------------------------------------
TADS_B_Aircraft* AircraftDataModel::FindOrCreateAircraft(unsigned int Addr, bool ACycleImages, int ANumSpriteImages)
{
    // 1. 해시테이블에서 항공기 검색
    TADS_B_Aircraft* aircraft = (TADS_B_Aircraft*)ght_get(FHashTable, sizeof(Addr), &Addr);

    // 2. 없으면 새로 생성 및 초기화
    if (!aircraft)
    {
        aircraft = new TADS_B_Aircraft;
        aircraft->ICAO = Addr;
        snprintf(aircraft->HexAddr, sizeof(aircraft->HexAddr), "%06X", (int)Addr);
        aircraft->NumMessagesSBS = 0;
        aircraft->NumMessagesRaw = 0;
        aircraft->VerticalRate = 0;
        aircraft->HaveAltitude = false;
        aircraft->HaveLatLon = false;
        aircraft->HaveSpeedAndHeading = false;
        aircraft->HaveFlightNum = false;
        aircraft->SpriteImage = FCurrentSpriteImage;
        if (ACycleImages)
        {
            FCurrentSpriteImage = (FCurrentSpriteImage + 1) % ANumSpriteImages;
        }

        // 3. 새로 생성한 객체를 해시테이블에 추가
        if (ght_insert(FHashTable, aircraft, sizeof(Addr), &Addr) < 0)
        {
            printf("ght_insert Error - Should Not Happen\n");
            // 실제라면 delete aircraft; 예외 발생 등의 처리가 필요
        }
    }
    return aircraft;
}
//---------------------------------------------------------------------------
void AircraftDataModel::ProcessRawMessage(const String& data, bool ACycleImages, int ANumSpriteImages)
{
    modeS_message mm;
    AnsiString ansiData = data; // Unicode -> Ansi 변환
    TDecodeStatus Status = decode_RAW_message(ansiData.c_str(), &mm);

    if (Status == HaveMsg)
    {
        uint32_t addr = (mm.AA[0] << 16) | (mm.AA[1] << 8) | mm.AA[2];
        TADS_B_Aircraft* ADS_B_Aircraft = FindOrCreateAircraft(addr, ACycleImages, ANumSpriteImages);
        RawToAircraft(&mm, ADS_B_Aircraft);
    }
    else
    {
        printf("Raw Decode Error:%d\n", Status);
    }
}
//---------------------------------------------------------------------------
void AircraftDataModel::ProcessSbsMessage(const String& data)
{
    // 참고: SBS_Message_Decode 함수가 전역변수(Form1->HashTable)에 직접 접근하지 않고,
    // 이 클래스의 FHashTable을 사용할 수 있도록 수정이 필요할 수 있습니다.
    // 예: SBS_Message_Decode(data.c_str(), FHashTable);
    AnsiString ansiData = data;
    SBS_Message_Decode(ansiData.c_str());
}
//---------------------------------------------------------------------------
void AircraftDataModel::PurgeStaleAircraft(int AStaleTimeSec)
{
    __int64 CurrentTime = GetCurrentTimeInMsec();
    __int64 StaleTimeInMs = AStaleTimeSec * 1000;
    
    ght_iterator_t iterator;
    const void* key;
    TADS_B_Aircraft* pData;

    for (pData = (TADS_B_Aircraft*)ght_first(FHashTable, &iterator, &key);
         pData;
         pData = (TADS_B_Aircraft*)ght_next(FHashTable, &iterator, &key))
    {
        if ((CurrentTime - pData->LastSeen) >= StaleTimeInMs)
        {
            void* pRemoved = ght_remove(FHashTable, sizeof(uint32_t), key);
            if(pRemoved)
            {
                delete static_cast<TADS_B_Aircraft*>(pRemoved);
            }
        }
    }
}
//---------------------------------------------------------------------------
void AircraftDataModel::PurgeAllAircraft()
{
    ght_iterator_t iterator;
    const void* key;
    void* pData;

    // for 루프와 ght_remove 호출이 모두 Model 클래스 내부에 존재
    // 안전한 순회를 위해 ght_next를 먼저 호출 후 ght_remove를 수행하도록 수정 가능
    for (pData = ght_first(FHashTable, &iterator, &key);
         pData;
         /* pData는 루프 안에서 다음 값으로 갱신됨 */)
    {
        TADS_B_Aircraft* currentAircraft = static_cast<TADS_B_Aircraft*>(pData);
        pData = ght_next(FHashTable, &iterator, &key); 
        
        void* pRemoved = ght_remove(FHashTable, sizeof(uint32_t), &currentAircraft->ICAO);
        if(pRemoved)
        {
            delete static_cast<TADS_B_Aircraft*>(pRemoved);
        }
    }
}
//---------------------------------------------------------------------------
int AircraftDataModel::GetAircraftCount()
{
    return ght_size(FHashTable);
}
//---------------------------------------------------------------------------
TADS_B_Aircraft* AircraftDataModel::GetFirstAircraft(ght_iterator_t* iterator, const void** key)
{
    return (TADS_B_Aircraft*)ght_first(FHashTable, iterator, key);
}
//---------------------------------------------------------------------------
TADS_B_Aircraft* AircraftDataModel::GetNextAircraft(ght_iterator_t* iterator, const void** key)
{
    return (TADS_B_Aircraft*)ght_next(FHashTable, iterator, key);
}
//---------------------------------------------------------------------------
TADS_B_Aircraft* AircraftDataModel::FindAircraftByICAO(unsigned int Addr)
{
    return (TADS_B_Aircraft*)ght_get(FHashTable, sizeof(Addr), &Addr);
}