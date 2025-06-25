#pragma once
#include <vector>
#include <functional>
#include <string>

template<typename T>
class IAviationProvider {
public:
    virtual ~IAviationProvider() {}

    // CSV 파일을 지정 경로로 다운로드 (예: airports.csv, routes.csv)
    virtual bool DownloadAndParseAviationData(const std::string& savePath) = 0;

    // 실시간 파싱: batch 단위로 콜백 (finished==true면 전체 완료)
    virtual void DownloadAndParseAviationDataAsync(
        std::function<void(const std::vector<T>&, bool finished)> onBatchParsed
    ) = 0;
};