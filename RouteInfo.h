#pragma once
#include <string>

class RouteInfo {
public:
    RouteInfo() = default;
    ~RouteInfo() = default;

    // Getters
	inline const std::string& GetCallsign() const { return Callsign; }
	inline const std::string& GetCode() const { return Code; }
	inline const std::string& GetNumber() const { return Number; }
	inline const std::string& GetAirlineCode() const { return AirlineCode; }
	inline const std::string& GetAirportCodes() const { return AirportCodes; }

	// Setters
	inline void SetCallsign(const std::string& v) { Callsign = v; }
	inline void SetCode(const std::string& v) { Code = v; }
	inline void SetNumber(const std::string& v) { Number = v; }
	inline void SetAirlineCode(const std::string& v) { AirlineCode = v; }
	inline void SetAirportCodes(const std::string& v) { AirportCodes = v; }

	// 비교 연산자 (==)
	inline bool operator==(const RouteInfo& other) const {
		return Callsign == other.Callsign &&
			   Code == other.Code &&
			   Number == other.Number &&
			   AirlineCode == other.AirlineCode &&
			   AirportCodes == other.AirportCodes;
	}

	// 비교 연산자 (!=)
	inline bool operator!=(const RouteInfo& other) const {
		return !(*this == other);
	}

private:
    std::string Callsign;
    std::string Code;
    std::string Number;
    std::string AirlineCode;
    std::string AirportCodes;
};