#pragma hdrstop

#include "TCPDataHandler.h"
#include "TimeFunctions.h"
#include "LogHandler.h"

const int MAX_RETRY_ATTEMPTS = 60;
const int RETRY_INTERVAL_MS = 1000;  // 1초

//---------------------------------------------------------------------------
__fastcall TCPDataHandler::TWorkerThread::TWorkerThread(TCPDataHandler* AHandler)
    : TThread(false), FHandler(AHandler) // 스레드를 생성-일시정지 상태(false)로 만듦
{
    FreeOnTerminate = true; // 스레드 종료 시 자동 메모리 해제
}
//---------------------------------------------------------------------------
// TWorkerThread의 실제 동작 내용
void __fastcall TCPDataHandler::TWorkerThread::Execute()
{
    if (FHandler->FDataSourceType == TDataSourceType::Network)
	{
		int retryCount = 0;
		bool connectionWasSuccessful = false;

		while (!Terminated) // 사용자가 Disconnect()를 호출하여 Terminated가 true가 될 때까지 반복
		{
            if (retryCount >= MAX_RETRY_ATTEMPTS)
			{
				// Synchronize를 사용하여 메인 스레드에서 ShowMessage를 안전하게 호출합니다.
				String connectionTypeStr = (FHandler->FConnectionType == TConnectionType::Raw) ? "Raw" : "SBS";
				Synchronize([&](){
					ShowMessage(connectionTypeStr + " connection failed. Please check the server and your network connection.");
				});
				break;
			}
			try
			{
				// Raw 연결에서만 재연결 시도 시 대기
				if (FHandler->FConnectionType == TConnectionType::Raw && retryCount > 0)
				{
					// UI에 "재연결 중" 상태 전송
					Synchronize([this](){ FHandler->SyncNotifyReconnecting(); });

					// 1초 대기
					TThread::Sleep(RETRY_INTERVAL_MS);
				}

				// 연결 시도
				FHandler->FClient->Host = FHandler->FHost;
				FHandler->FClient->Port = FHandler->FPort;
				FHandler->FClient->ConnectTimeout = 3000; // 3초 타임아웃 설정
				FHandler->FClient->Connect(); // 여기서 실패하면 catch 블록으로 이동
				
				FHandler->FClient->Socket->Binding->SetKeepAliveValues(true, 3000, 1000);

				connectionWasSuccessful = true;
                Synchronize([this](){ FHandler->SyncNotifyConnected(); });				

				// 데이터 수신 루프
				while (!Terminated && FHandler->FClient->Connected())
				{
					try {
						String data = FHandler->FClient->IOHandler->ReadLn();
						if (!Terminated) {
							TThread::Queue(nullptr, [this, data](){ FHandler->SyncNotifyData(data); });
						}
					}
					catch (const Exception& readException) {
						// 연결 상태 재확인
						if (!FHandler->FClient->Connected()) {
							// 연결이 끊어진 경우 즉시 루프 종료하여 재연결 시도
							break;
						}
						
						// Raw 연결에서는 연결이 살아있을 때만 ReadLn 재시도
						if (FHandler->FConnectionType == TConnectionType::Raw) {
							// 짧은 대기 후 계속 시도
							TThread::Sleep(100);
							continue;
						}
						// SBS 연결에서는 ReadLn 예외 시 즉시 종료
						else {
							throw; // 예외를 다시 던져서 외부 catch에서 처리
						}
					}
				}
				retryCount = 0;
			}
			catch (const Exception& e)
			{
				// 연결이 끊기면 예외가 발생합니다.
				if (Terminated) {
					break; // 사용자가 취소한 것이므로 즉시 루프 종료
				}
				
				// 연결이 성공했다가 끊긴 경우에도 Raw는 재시도, SBS는 즉시 종료
				if (connectionWasSuccessful) {
					connectionWasSuccessful = false; // 재시도를 위해 플래그 리셋
					if (FHandler->FConnectionType == TConnectionType::SBS) {
						// SBS는 재시도하지 않으므로 즉시 에러 메시지 표시
						Synchronize([&](){
							ShowMessage("SBS connection lost: " + e.Message);
						});
						break; // SBS는 즉시 루프 종료
					}
					// Raw의 경우 재시도 로직으로 계속 진행
				}
				// Raw 연결에서만 재시도 횟수를 증가시키고 루프를 계속
				if (FHandler->FConnectionType == TConnectionType::Raw) {
					retryCount++;
					if (FHandler->FClient->Connected()) {
						FHandler->FClient->Disconnect();
					}
					continue; // Raw는 재시도를 위해 루프 계속
				}
				// SBS 연결의 경우 즉시 종료 (재시도 없음)
				else {
					if (FHandler->FClient->Connected()) {
						FHandler->FClient->Disconnect();
					}
					// SBS는 재시도하지 않으므로 즉시 에러 메시지 표시
					Synchronize([&](){
						ShowMessage("SBS connection failed: " + e.Message);
					});
					break; // SBS는 즉시 루프 종료
				}
			}
		}
	}
	else // File Playback 로직
	{
        try
		{
			// 파일이 없거나 접근 불가 시 여기서 예외 발생 가능
			FHandler->FPlaybackStream = new TStreamReader(FHandler->FPlaybackFileName);
			Synchronize([this](){ FHandler->SyncNotifyConnected(); });

			bool firstPlaybackLine = true;
			__int64 lastTime = 0;

			while (!Terminated && !FHandler->FPlaybackStream->EndOfStream)
			{
				String timeStr = FHandler->FPlaybackStream->ReadLine();
				if (FHandler->FPlaybackStream->EndOfStream) break;
				AnsiString msgStr = FHandler->FPlaybackStream->ReadLine();

				// 파일 포맷이 잘못된 경우 여기서 예외 발생 가능
				__int64 currentTime = StrToInt64(timeStr);

				if (firstPlaybackLine)
				{
					firstPlaybackLine = false;
					lastTime = currentTime;
				}

				__int64 sleepTime = currentTime - lastTime;
				if (FHandler->FPlaybackSpeed > 0.0) {
                    sleepTime = static_cast<__int64>(sleepTime / FHandler->FPlaybackSpeed);
                }
                if (sleepTime > 0 && sleepTime < 10000)
				{
					TThread::Sleep(sleepTime);
				}
				lastTime = currentTime;

				if (!Terminated) {
					TThread::Queue(nullptr, [this, msgStr](){ FHandler->SyncNotifyData(msgStr); });
				}
			}
		}
		catch (const Exception& e)
		{
			// 파일 관련 예외가 발생했을 때 처리하는 부분
			Synchronize([&](){
				// 메인 스레드에서 안전하게 에러 메시지 표시
				ShowMessage("Error during file playback: " + e.Message);
			});
		}
	}

    if (FHandler->FClient && FHandler->FClient->Connected()) {
		FHandler->FClient->Disconnect();
	}
	if (FHandler->FPlaybackStream) {
		delete FHandler->FPlaybackStream;
		FHandler->FPlaybackStream = NULL;
	}
}

//---------------------------------------------------------------------------
// TCPDataHandler 클래스 구현 (이제 TThread 대신 TWorkerThread를 사용)
//---------------------------------------------------------------------------
__fastcall TCPDataHandler::TCPDataHandler(TComponent* AOwner)
    : TObject(), FOwner(AOwner), FClient(NULL), FWorkerThread(NULL),
      FPlaybackStream(NULL), FRecordStream(NULL), FIsActive(false),
      FConnectionType(TConnectionType::Raw)
{
    FClient = new TIdTCPClient(nullptr);
    
    // LogHandler 초기화 및 NETWORK 카테고리 활성화
    LogHandler::Initialize();
    LogHandler::EnableCategory(LogHandler::CAT_NETWORK);
    LogHandler::SetConsoleOutput(true);  // 콘솔 출력 활성화
}
//---------------------------------------------------------------------------
__fastcall TCPDataHandler::~TCPDataHandler()
{
    if (FWorkerThread) {
		FWorkerThread->Terminate();
	}
	delete FClient;
	StopRecording();
}
//---------------------------------------------------------------------------
void TCPDataHandler::Connect(const String& host, int port, TConnectionType connectionType)
{
    if (FIsActive) return;
    FHost = host;
    FPort = port;
    FConnectionType = connectionType;
    FDataSourceType = TDataSourceType::Network;
    FIsActive = true;

    FWorkerThread = new TWorkerThread(this);
    FWorkerThread->FreeOnTerminate = true;
    FWorkerThread->OnTerminate = ThreadTerminated;
	FWorkerThread->Resume();
}
//---------------------------------------------------------------------------
void TCPDataHandler::StartPlayback(const String& fileName)
{
    if (FIsActive) return;
    FPlaybackFileName = fileName;
    FDataSourceType = TDataSourceType::File;
    FIsActive = true;

    FWorkerThread = new TWorkerThread(this);
    FWorkerThread->FreeOnTerminate = true;
    FWorkerThread->OnTerminate = ThreadTerminated;
    FWorkerThread->Resume();
}
//---------------------------------------------------------------------------
void TCPDataHandler::Disconnect()
{
	if (FWorkerThread)
	{
		FWorkerThread->Terminate();
	}

    if (FClient)
	{
		try
		{
			// 이 호출은 다른 스레드에서 실행 중인 Connect()를 즉시 중단시키고, 예외를 발생시킵니다.
			FClient->Disconnect();
		}
		catch(...)
		{
			// 예외는 무시합니다. 목적은 다른 스레드를 깨우는 것이기 때문입니다.
		}
	}

    // 3. UI의 즉각적인 반응을 위한 추가 처리
    if (FIsActive)
	{
		FIsActive = false;
		if (OnDisconnected) {
			OnDisconnected("User disconnected");
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TCPDataHandler::ThreadTerminated(TObject* Sender)
{
	FWorkerThread = NULL;

	if (FIsActive)
	{
		FIsActive = false;
		if (OnDisconnected) {
			OnDisconnected("Connection lost or terminated.");
		}
	}
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
    FIsActive = true;
	if (OnConnected) OnConnected();
}
//---------------------------------------------------------------------------
void __fastcall TCPDataHandler::SyncNotifyReconnecting()
{
    if (OnReconnecting) OnReconnecting();
}
//---------------------------------------------------------------------------