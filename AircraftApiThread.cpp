#include <vcl.h>
#pragma hdrstop

#include "AircraftApiThread.h"
#include "AircraftApi.h"
#include "DisplayGUI.h"

__fastcall TLoadApiDataThread::TLoadApiDataThread()
    : TThread(false) // 생성 즉시 실행
{
    FreeOnTerminate = true;
}

void __fastcall TLoadApiDataThread::NotifyUI() {
    Form1->ObjectDisplay->Repaint();
}

void __fastcall TLoadApiDataThread::Execute() {
    try {
        printf("[Thread] LoadAllData() 시작\n");
        LoadAllData();
        Form1->InitRouteAirportMaps();
        printf("[Thread] LoadAllData() 완료\n");

        TThread::Synchronize(nullptr, NotifyUI);
    } catch (const std::exception& e) {
        printf("[Thread] 예외: %s\n", e.what());
    }
}