#include <System.Net.HttpClient.hpp>
#include <System.Net.URLClient.hpp>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include "AircraftApi.h"

//#define DEBUG_KANG

// 실제 전역 변수 정의 (중복 정의 주의!)
std::vector<AirportInfo> apiAirportList;
std::vector<RouteInfo> apiRouteList;

// CSV 한 줄 파싱 (쉼표, 따옴표 처리)
static std::vector<std::string> split_csv_line(const std::string& line) {
    std::vector<std::string> result;
    std::string cell;
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') in_quotes = !in_quotes;
        else if (c == ',' && !in_quotes) {
            result.push_back(cell);
            cell.clear();
        } else {
            cell += c;
        }
    }
    result.push_back(cell);
    return result;
}

// 공항 코드 필드 분리 (공항1-공항2-공항3)
static std::vector<std::string> split_airport_codes(const std::string& s) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, '-')) {
        result.push_back(item);
    }
    return result;
}

// 공항 정보 파싱 및 로딩
std::vector<AirportInfo> FetchAirportList() {
    std::vector<AirportInfo> result;

    auto http = std::unique_ptr<THTTPClient>(THTTPClient::Create());
    http->ConnectionTimeout = 5000;
    http->ResponseTimeout = 5000;

    try {
        String url = "https://vrs-standing-data.adsb.lol/airports.csv";
        String data = http->Get(url)->ContentAsString();

        std::string response = AnsiString(data).c_str();
        std::istringstream ss(response);
        std::string line;
        size_t line_no = 0;

        // 첫 줄은 헤더
        std::getline(ss, line); line_no++;

        while (std::getline(ss, line)) {
            line_no++;
            auto cells = split_csv_line(line);
            if (cells.size() < 9) {
                std::cerr << "[Airport CSV] 파싱 실패: 라인 " << line_no << " [" << line << "]\n";
                continue;
            }

            try {
                AirportInfo info;
                info.code        = cells[0];
                info.name        = cells[1];
                info.icao        = cells[2];
                info.iata        = cells[3];
                info.location    = cells[4];
                info.countryIso2 = cells[5];
                info.latitude    = cells[6].empty() ? 0.0 : std::stod(cells[6]);
                info.longitude   = cells[7].empty() ? 0.0 : std::stod(cells[7]);
                info.altitudeFeet= cells[8].empty() ? 0   : std::stoi(cells[8]);
                result.push_back(info);
            } catch (const std::exception& e) {
                std::cerr << "[Airport CSV] 변환 오류: 라인 " << line_no << " [" << line << "] - " << e.what() << "\n";
                continue;
            }
        }
    } catch (const Exception& e) {
        std::wcerr << L"[Airport CSV] 예외 발생: " << e.Message.c_str() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Airport CSV] 표준 예외: " << e.what() << std::endl;
    }

    return result;
}

// 루트 정보 파싱 및 로딩
std::vector<RouteInfo> FetchRouteList() {
    std::vector<RouteInfo> result;

    auto http = std::unique_ptr<THTTPClient>(THTTPClient::Create());
    http->ConnectionTimeout = 5000;
    http->ResponseTimeout = 5000;

    try {
        String url = "https://vrs-standing-data.adsb.lol/routes.csv";
        String data = http->Get(url)->ContentAsString();

        std::string response = AnsiString(data).c_str();
        std::istringstream ss(response);
        std::string line;
        size_t line_no = 0;

        std::getline(ss, line); line_no++;

        while (std::getline(ss, line)) {
            line_no++;
            auto cells = split_csv_line(line);
            if (cells.size() < 5) {
                std::cerr << "[Route CSV] 파싱 실패: 라인 " << line_no << " [" << line << "]\n";
                continue;
            }

            try {
                RouteInfo info;
                info.callSign     = cells[0];
                info.code         = cells[1];
                info.number       = cells[2];
                info.airlineCode  = cells[3];
                info.airportCodes = split_airport_codes(cells[4]);
                result.push_back(info);
            } catch (const std::exception& e) {
                std::cerr << "[Route CSV] 변환 오류: 라인 " << line_no << " [" << line << "] - " << e.what() << "\n";
                continue;
            }
        }
    } catch (const Exception& e) {
        std::wcerr << L"[Route CSV] 예외 발생: " << e.Message.c_str() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Route CSV] 표준 예외: " << e.what() << std::endl;
    }

    return result;
}

#ifdef DEBUG_KANG
// LoadAllData 이후 호출 (ex: FormCreate 등에서 사용 가능)
void PrintApiDataForTest() {
    printf("=== Airport List (%zu items) ===\n", apiAirportList.size());
    for (size_t i = 0; i < 10; ++i) {
        const auto& airport = apiAirportList[i];
        printf("[%zu] Code: %s, Name: %s, ICAO: %s, Country: %s, Lat: %.4f, Lon: %.4f\n",
               i,
               airport.code.c_str(),
               airport.name.c_str(),
               airport.icao.c_str(),
               airport.countryIso2.c_str(),
               airport.latitude,
               airport.longitude);
    }

    printf("=== Route List (%zu items) ===\n", apiRouteList.size());
    for (size_t i = 0; i < 10; ++i) {
        const auto& route = apiRouteList[i];
        printf("[%zu] CallSign: %s, Code: %s, Number: %s, Airline: %s, Airports: ",
               i,
               route.callSign.c_str(),
               route.code.c_str(),
               route.number.c_str(),
               route.airlineCode.c_str());
        for (size_t j = 0; j < route.airportCodes.size(); ++j) {
            printf("%s", route.airportCodes[j].c_str());
            if (j < route.airportCodes.size() - 1) printf(" -> ");
        }
        printf("\n");
    }
}
#endif

// 모든 데이터 한번에 로딩
void LoadAllData() {
    printf("LoadAllData() START\n");
    apiAirportList = FetchAirportList();
    apiRouteList = FetchRouteList();
    printf("[AircraftApi] Loaded: airport=%zu, route=%zu\n", apiAirportList.size(), apiRouteList.size());
    printf("LoadAllData() END\n");
#ifdef DEBUG_KANG
    PrintApiDataForTest();
#endif
}


