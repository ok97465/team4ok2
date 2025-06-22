#pragma hdrstop

#include "TCPDataHandler.h"
#include "TimeFunctions.h" // <<< GetCurrentTimeInMsec 사용을 위해 추가

//---------------------------------------------------------------------------
// TWorkerThread 생성자
__fastcall TCPDataHandler::TWorkerThread::TWorkerThread(TCPDataHandler* AHandler)
    : TThread(false), FHandler(AHandler) // 스레드를 생성-일시정지 상태(false)로 만듦
{
    FreeOnTerminate = true; // 스레드 종료 시 자동 메모리 해제
}
//---------------------------------------------------------------------------
// TWorkerThread의 실제 동작 내용
void __fastcall TCPDataHandler::TWorkerThread::Execute()
{
    // 이 메소드 안에서는 Terminated 속성에 직접 접근 가능
    String data;
    bool firstPlaybackLine = true;
    __int64 lastTime = 0;

    try
    {
        if (FHandler->FDataSourceType == TDataSourceType::Network)
        {
            FHandler->FClient = new TIdTCPClient(FHandler->FOwner);
            FHandler->FClient->Host = FHandler->FHost;
            FHandler->FClient->Port = FHandler->FPort;
            FHandler->FClient->Connect();
            FHandler->FClient->Socket->Binding->SetKeepAliveValues(true, 60 * 1000, 15 * 1000);
            Synchronize([this](){ FHandler->SyncNotifyConnected(); });

            while (!Terminated)
            {
                data = FHandler->FClient->IOHandler->ReadLn();
                Synchronize([this, data](){ FHandler->SyncNotifyData(data); });
            }
        }
        else // File Playback
        {
            FHandler->FPlaybackStream = new TStreamReader(FHandler->FPlaybackFileName);
            Synchronize([this](){ FHandler->SyncNotifyConnected(); });

            while (!Terminated && !FHandler->FPlaybackStream->EndOfStream)
            {
                String timeStr = FHandler->FPlaybackStream->ReadLine();
                if (FHandler->FPlaybackStream->EndOfStream) break;
                AnsiString msgStr = FHandler->FPlaybackStream->ReadLine();

                __int64 currentTime = StrToInt64(timeStr);
                if (firstPlaybackLine)
                {
                    firstPlaybackLine = false;
                    lastTime = currentTime;
                }

                __int64 sleepTime = currentTime - lastTime;
                if (sleepTime > 0 && sleepTime < 10000) // 비정상적인 sleep 방지
                {
                    TThread::Sleep(sleepTime);
                }
                lastTime = currentTime;

                Synchronize([this, msgStr](){ FHandler->SyncNotifyData(msgStr); });
            }
        }
    }
    catch (const Exception& e)
    {
        ShowMessage("Error while connecting: "+e.Message);
    }

    // 스레드 종료 처리
    if (FHandler->FClient)
    {
        FHandler->FClient->Disconnect();
        delete FHandler->FClient;
        FHandler->FClient = NULL;
    }
    if (FHandler->FPlaybackStream)
    {
        delete FHandler->FPlaybackStream;
        FHandler->FPlaybackStream = NULL;
    }
    FHandler->FIsActive = false;
    Synchronize([this](){ FHandler->SyncNotifyDisconnected("Normal termination"); });
}

//---------------------------------------------------------------------------
// TCPDataHandler 클래스 구현 (이제 TThread 대신 TWorkerThread를 사용)
//---------------------------------------------------------------------------
__fastcall TCPDataHandler::TCPDataHandler(TComponent* AOwner)
    : TObject(), FOwner(AOwner), FClient(NULL), FWorkerThread(NULL),
      FPlaybackStream(NULL), FRecordStream(NULL), FIsActive(false)
{
}
//---------------------------------------------------------------------------
__fastcall TCPDataHandler::~TCPDataHandler()
{
    Disconnect();
    StopRecording();
}
//---------------------------------------------------------------------------
void TCPDataHandler::Connect(const String& host, int port)
{
    if (FIsActive) return;
    FHost = host;
    FPort = port;
    FDataSourceType = TDataSourceType::Network;
    FIsActive = true;

    // new TThread(false) -> new TWorkerThread(this) 로 변경
    FWorkerThread = new TWorkerThread(this);
    FWorkerThread->FreeOnTerminate=true;
	FWorkerThread->Resume(); // Resume() 대신 Start()가 표준
}
//---------------------------------------------------------------------------
void TCPDataHandler::StartPlayback(const String& fileName)
{
    if (FIsActive) return;
    FPlaybackFileName = fileName;
    FDataSourceType = TDataSourceType::File;
    FIsActive = true;

    // new TThread(false) -> new TWorkerThread(this) 로 변경
    FWorkerThread = new TWorkerThread(this);
    FWorkerThread->FreeOnTerminate=true;
    FWorkerThread->Resume();
}
//---------------------------------------------------------------------------
void TCPDataHandler::Disconnect()
{
    if (!FIsActive || !FWorkerThread) return;
    FWorkerThread->Terminate();
}
//---------------------------------------------------------------------------
void TCPDataHandler::StartRecording(const String& fileName)
{
    StopRecording();
    try
    {
        FRecordStream = new TStreamWriter(fileName, false, TEncoding::UTF8, 4096);
    }
    catch(...)
    {
        FRecordStream = NULL;
    }
}
//---------------------------------------------------------------------------
void TCPDataHandler::StopRecording()
{
    if (FRecordStream)
    {
        delete FRecordStream;
        FRecordStream = NULL;
    }
}
//---------------------------------------------------------------------------
// 동기화 메소드들 (변경 없음)
//---------------------------------------------------------------------------
void __fastcall TCPDataHandler::SyncNotifyData(AnsiString data)
{
    if (FRecordStream)
    {
        FRecordStream->WriteLine(IntToStr(GetCurrentTimeInMsec()));
        FRecordStream->WriteLine(data);
    }

    if (OnDataReceived)
    {
        OnDataReceived(data);
    }
}
//---------------------------------------------------------------------------
void __fastcall TCPDataHandler::SyncNotifyConnected()
{
    if (OnConnected)
    {
        OnConnected();
    }
}
//---------------------------------------------------------------------------
void __fastcall TCPDataHandler::SyncNotifyDisconnected(String reason)
{
    if (OnDisconnected)
    {
        OnDisconnected(reason);
    }
}