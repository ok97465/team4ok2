#pragma once
#include <string>
#include <vector>

struct RouteInfo {
    std::string callSign;               // Callsign
    std::string code;                   // Code
    std::string number;                 // Number
    std::string airlineCode;            // AirlineCode
    std::vector<std::string> airportCodes; // 공항 코드들
};

struct AirportInfo {
    std::string code;
    std::string name;
    std::string icao;
    std::string iata;
    std::string location;
    std::string countryIso2;
    double latitude;
    double longitude;
    int altitudeFeet;
};