#include "RouteProvider.h"
#include <windows.h>
#include <wininet.h>
#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include "LogHandler.h"

RouteProvider::RouteProvider() {}
RouteProvider::~RouteProvider() {}

void RouteProvider::DownloadAndParseAviationDataAsync(
    std::function<void(const std::vector<RouteInfo>&, bool finished)> onBatchParsed)
{
    buffer.clear();
    downloadFinished = false;
    downloadSuccess = false;

    // 파싱 스레드
    std::thread parsingThread([this, onBatchParsed]() {
        std::vector<RouteInfo> batch;
        std::string incompleteLine;
        bool isHeader = true;
        std::vector<int> colIdx(5, -1);
        auto lastBatchTime = std::chrono::steady_clock::now();

        while (true) {
            std::string localBuffer;
            bool localDownloadFinished = false;

            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait_for(lock, std::chrono::milliseconds(100), [this] { return !buffer.empty() || downloadFinished; });
                localBuffer.swap(buffer);
                localDownloadFinished = downloadFinished;
            }

            // incompleteLine에서 이어서 붙임
            if (!incompleteLine.empty()) {
                localBuffer = incompleteLine + localBuffer;
                incompleteLine.clear();
            }

            std::istringstream ss(localBuffer);
            std::string line;
            while (std::getline(ss, line)) {
                if (ss.eof() && localBuffer.back() != '\n' && localBuffer.back() != '\r') {
                    incompleteLine = line;
                    break;
                }
                std::vector<std::string> cols;
                std::string cell;
                std::istringstream ls(line);
                while (std::getline(ls, cell, ',')) {
                    cols.push_back(cell);
                }
                if (isHeader) {
                    for (size_t i = 0; i < cols.size(); ++i) {
                        if (cols[i] == "Callsign") colIdx[0] = i;
                        else if (cols[i] == "Code") colIdx[1] = i;
                        else if (cols[i] == "Number") colIdx[2] = i;
                        else if (cols[i] == "AirlineCode") colIdx[3] = i;
                        else if (cols[i] == "AirportCodes") colIdx[4] = i;
                    }
                    isHeader = false;
                    continue;
                }
                if (cols.size() < 5) continue;
                RouteInfo info;
                if (colIdx[0] >= 0 && colIdx[0] < (int)cols.size()) info.SetCallsign(cols[colIdx[0]]);
                if (colIdx[1] >= 0 && colIdx[1] < (int)cols.size()) info.SetCode(cols[colIdx[1]]);
                if (colIdx[2] >= 0 && colIdx[2] < (int)cols.size()) info.SetNumber(cols[colIdx[2]]);
                if (colIdx[3] >= 0 && colIdx[3] < (int)cols.size()) info.SetAirlineCode(cols[colIdx[3]]);
                if (colIdx[4] >= 0 && colIdx[4] < (int)cols.size()) info.SetAirportCodes(cols[colIdx[4]]);
                batch.push_back(info);

                // 100개마다 콜백
				if (batch.size() >= 100) {
#if 0
                std::cout << "[Route Batch Parsed] size=" << batch.size() << std::endl;
                for (const auto& r : batch) {
                    std::cout << r.GetCallsign() << ", " << r.GetCode() << ", " << r.GetNumber() << ", "
                              << r.GetAirlineCode() << ", " << r.GetAirportCodes() << std::endl;
                }
#endif
					onBatchParsed(batch, false);
					batch.clear();
					lastBatchTime = std::chrono::steady_clock::now();
				}
			}

			// 100개가 안되더라도 0.5초마다 콜백
			auto now = std::chrono::steady_clock::now();
			if (!batch.empty() && std::chrono::duration_cast<std::chrono::milliseconds>(now - lastBatchTime).count() >= 500) {
#if 0
				std::cout << "[Route Batch Parsed] size=" << batch.size() << std::endl;
				for (const auto& r : batch) {
						std::cout << r.GetCallsign() << ", " << r.GetCode() << ", " << r.GetNumber() << ", "
							  << r.GetAirlineCode() << ", " << r.GetAirportCodes() << std::endl;
				}
#endif
				onBatchParsed(batch, false);
				batch.clear();
				lastBatchTime = now;
			}

			if (localDownloadFinished) {
				// 남은 incompleteLine 처리
                if (!incompleteLine.empty()) {
                    std::string lastLine = incompleteLine;
                    incompleteLine.clear();
                    std::vector<std::string> cols;
                    std::string cell;
                    std::istringstream ls(lastLine);
                    while (std::getline(ls, cell, ',')) {
                        cols.push_back(cell);
                    }
                    if (!isHeader && cols.size() >= 5) {
                        RouteInfo info;
                        if (colIdx[0] >= 0 && colIdx[0] < (int)cols.size()) info.SetCallsign(cols[colIdx[0]]);
                        if (colIdx[1] >= 0 && colIdx[1] < (int)cols.size()) info.SetCode(cols[colIdx[1]]);
                        if (colIdx[2] >= 0 && colIdx[2] < (int)cols.size()) info.SetNumber(cols[colIdx[2]]);
                        if (colIdx[3] >= 0 && colIdx[3] < (int)cols.size()) info.SetAirlineCode(cols[colIdx[3]]);
                        if (colIdx[4] >= 0 && colIdx[4] < (int)cols.size()) info.SetAirportCodes(cols[colIdx[4]]);
                        batch.push_back(info);
                    }
                }
                // 남은 데이터도 콜백
                if (!batch.empty()) {
                    LOG_DEBUG_F(LogHandler::CAT_GENERAL, "[Route Batch Parsed] size=%zu", batch.size());
                    for (const auto& r : batch) {
                        LOG_DEBUG_F(LogHandler::CAT_GENERAL, "%s, %s, %s, %s, %s", 
                                  r.GetCallsign().c_str(), r.GetCode().c_str(), r.GetNumber().c_str(),
                                  r.GetAirlineCode().c_str(), r.GetAirportCodes().c_str());
                    }
                    onBatchParsed(batch, false);
                    batch.clear();
                }
                // 파싱 완료 콜백
                onBatchParsed({}, true);
                break;
            }
        }
    });

    // 다운로드 스레드
    std::thread downloadThread([this]() {
        const char* url = "https://vrs-standing-data.adsb.lol/routes.csv";
        HINTERNET hInternet = InternetOpenA("RouteProvider", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hInternet) {
            std::lock_guard<std::mutex> lock(mtx);
            downloadFinished = true;
            downloadSuccess = false;
            cv.notify_all();
            return;
        }

        HINTERNET hFile = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hFile) {
            InternetCloseHandle(hInternet);
            std::lock_guard<std::mutex> lock(mtx);
            downloadFinished = true;
            downloadSuccess = false;
            cv.notify_all();
            return;
        }

        char bufferChunk[4096];
        DWORD bytesRead = 0;
        BOOL bResult = TRUE;
        while (bResult) {
            bResult = InternetReadFile(hFile, bufferChunk, sizeof(bufferChunk), &bytesRead);
            if (bytesRead == 0) break;
            {
                std::lock_guard<std::mutex> lock(mtx);
                buffer.append(bufferChunk, bytesRead);
                cv.notify_all();
            }
        }

        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);

        std::lock_guard<std::mutex> lock(mtx);
        downloadFinished = true;
        downloadSuccess = true;
        cv.notify_all();
    });

    downloadThread.detach();
    parsingThread.detach();
}

// routes.csv를 지정 경로로 다운로드
bool RouteProvider::DownloadAndParseAviationData(const std::string& savePath) {
    const char* url = "https://vrs-standing-data.adsb.lol/routes.csv";
    HINTERNET hInternet = InternetOpenA("RouteProvider", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) return false;

    HINTERNET hFile = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile) {
        InternetCloseHandle(hInternet);
        return false;
    }

    std::ofstream outFile(savePath, std::ios::binary);
    if (!outFile.is_open()) {
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return false;
    }

    char buffer[4096];
    DWORD bytesRead = 0;
    BOOL bResult = TRUE;
    while (bResult) {
        bResult = InternetReadFile(hFile, buffer, sizeof(buffer), &bytesRead);
        if (bytesRead == 0) break;
        outFile.write(buffer, bytesRead);
    }

    outFile.close();
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    return true;
}