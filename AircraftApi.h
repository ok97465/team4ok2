#pragma once
#include "AircraftInfo.h"
#include <vector>

std::vector<AirportInfo> FetchAirportList();
std::vector<RouteInfo> FetchRouteList();
void LoadAllData();

// **외부에서 참조할 수 있도록 extern으로 선언**
extern std::vector<AirportInfo> apiAirportList;
extern std::vector<RouteInfo> apiRouteList;