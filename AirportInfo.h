#pragma once
#include <string>

class AirportInfo {
public:
    AirportInfo() = default;
    ~AirportInfo() = default;

    // Getters
	inline const std::string& GetCode() const { return Code; }
	inline const std::string& GetName() const { return Name; }
	inline const std::string& GetICAO() const { return ICAO; }
	inline const std::string& GetIATA() const { return IATA; }
	inline const std::string& GetLocation() const { return Location; }
	inline const std::string& GetCountryISO2() const { return CountryISO2; }
	inline const std::string& GetLatitude() const { return Latitude; }
	inline const std::string& GetLongitude() const { return Longitude; }
	inline const std::string& GetAltitudeFeet() const { return AltitudeFeet; }

	// Setters
	inline void SetCode(const std::string& v) { Code = v; }
	inline void SetName(const std::string& v) { Name = v; }
	inline void SetICAO(const std::string& v) { ICAO = v; }
	inline void SetIATA(const std::string& v) { IATA = v; }
	inline void SetLocation(const std::string& v) { Location = v; }
	inline void SetCountryISO2(const std::string& v) { CountryISO2 = v; }
	inline void SetLatitude(const std::string& v) { Latitude = v; }
	inline void SetLongitude(const std::string& v) { Longitude = v; }
	inline void SetAltitudeFeet(const std::string& v) { AltitudeFeet = v; }

	// 비교 연산자 (==)
	inline bool operator==(const AirportInfo& other) const {
        return Code == other.Code &&
               Name == other.Name &&
               ICAO == other.ICAO &&
               IATA == other.IATA &&
               Location == other.Location &&
               CountryISO2 == other.CountryISO2 &&
               Latitude == other.Latitude &&
               Longitude == other.Longitude &&
               AltitudeFeet == other.AltitudeFeet;
    }

    // 비교 연산자 (!=)
	inline bool operator!=(const AirportInfo& other) const {
        return !(*this == other);
    }

private:
    std::string Code;
    std::string Name;
    std::string ICAO;
    std::string IATA;
    std::string Location;
    std::string CountryISO2;
    std::string Latitude;
    std::string Longitude;
    std::string AltitudeFeet;
};