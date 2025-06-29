"""
ADS-B 시뮬레이션에 필요한 공통 유틸리티 함수들을 모아놓은 모듈.

이 모듈은 다음과 같은 기능을 제공합니다:
- 지리적 계산: 두 지점 간의 방위, 거리, 다음 위치 계산
- 데이터 로딩: CSV 파일에서 공항 및 경로 데이터 로딩
- SBS 메시지 생성: 표준 SBS-1 형식에 맞는 메시지 문자열 생성
"""

# Standard Libraries
import csv
import os
from datetime import datetime
from random import choice, choices, randint

# Third Party Libraries
from numpy import arcsin, arctan2, cos, deg2rad, rad2deg, sin


def load_data(filepath):
    """CSV 파일에서 데이터를 읽어 리스트 또는 딕셔너리로 반환합니다."""
    if not os.path.exists(filepath):
        print(f"오류: {filepath} 파일을 찾을 수 없습니다. 경로를 확인하세요.")
        return None
    try:
        with open(filepath, mode="r", encoding="utf-8-sig") as infile:
            reader = csv.DictReader(infile)
            if "ICAO" in reader.fieldnames and "Latitude" in reader.fieldnames:
                return {
                    row["ICAO"].strip(): (
                        float(row["Latitude"]),
                        float(row["Longitude"]),
                    )
                    for row in reader
                    if row["ICAO"] and row["Latitude"] and row["Longitude"]
                }
            else:
                data = list(reader)
                for row in data:
                    if "AirportCodes" in row and row["AirportCodes"]:
                        row["AirportCodes"] = row["AirportCodes"].strip()
                return data
    except Exception as e:
        print(f"오류: {filepath} 파일을 읽는 중 오류 발생: {e}")
        return None


def load_icao24_list(filepath):
    icao24_list = []
    try:
        with open(filepath, mode="r", encoding="utf-8-sig") as infile:
            reader = csv.DictReader(infile)
            for row in reader:
                icao = row.get("icao24", "").strip().upper()
                if (
                    icao
                    and len(icao) == 6
                    and all(c in "0123456789ABCDEF" for c in icao)
                ):
                    icao24_list.append(icao)
    except Exception as e:
        print(f"오류: {filepath} 파일을 읽는 중 오류 발생: {e}")
    return icao24_list


def calculate_initial_bearing(lat1, lon1, lat2, lon2):
    """두 지점 간의 초기 방위를 계산합니다."""
    lat1, lon1, lat2, lon2 = map(deg2rad, [lat1, lon1, lat2, lon2])
    dLon = lon2 - lon1
    x = sin(dLon) * cos(lat2)
    y = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon)
    initial_bearing = arctan2(x, y)
    return (rad2deg(initial_bearing) + 360) % 360


def calculate_next_position(lat, lon, bearing, distance_nm):
    """현재 위치, 방위, 거리를 기반으로 다음 위치를 계산합니다."""
    R = 3440.065  # 지구의 반경 (해리)
    lat, lon, bearing = map(deg2rad, [lat, lon, bearing])
    d = distance_nm / R

    lat2 = arcsin(sin(lat) * cos(d) + cos(lat) * sin(d) * cos(bearing))
    lon2 = lon + arctan2(
        sin(bearing) * sin(d) * cos(lat), cos(d) - sin(lat) * sin(lat2)
    )
    return rad2deg(lat2), rad2deg(lon2)


def calculate_distance_nm(lat1, lon1, lat2, lon2):
    """두 지점 간의 거리를 해리(nautical miles)로 계산합니다."""
    R = 3440.065
    lat1_rad, lon1_rad, lat2_rad, lon2_rad = map(deg2rad, [lat1, lon1, lat2, lon2])
    dlon = lon2_rad - lon1_rad
    dlat = lat2_rad - lat1_rad
    a = sin(dlat / 2) ** 2 + cos(lat1_rad) * cos(lat2_rad) * sin(dlon / 2) ** 2
    c = 2 * arctan2(a**0.5, (1 - a) ** 0.5)
    return R * c


def generate_hexid(n: int) -> list[str]:
    """항공기가 제작되면 부여되는 고유번호를 생성한다."""
    return ["".join(choices("0123456789ABCDEF", k=6)) for _ in range(n)]


def generate_callsign(n: int) -> list[str]:
    """항공기 경로에 따라 부여되는 callsign을 생성한다."""
    prefix_list = ["AAL", "KAL", "DAL", "UAL", "BAW", "JAL"]
    return [choice(prefix_list) + str(randint(100, 9999)) for _ in range(n)]


def format_sbs_messages(timestamp, date_str, time_str, ac_data):
    """주어진 항공기 데이터로 SBS-1 메시지 3종(MSG,1,3,4)을 포맷팅합니다."""
    hex_id = ac_data["hex_id"]
    callsign = ac_data["callsign"]
    altitude = ac_data["altitude"]
    lat = ac_data["lat"]
    lon = ac_data["lon"]
    speed = ac_data["speed"]
    bearing = ac_data["bearing"]

    # 00_generate_sbs.py, 01_generate_sbs.py와 동일한 필드/콤마 개수로 맞춤
    msg1 = (
        f"{timestamp}\nMSG,1,0,0,{hex_id},0,{date_str},{time_str},{date_str},{time_str},{callsign},,,,,,,,,,,\n"
    )
    msg3 = (
        f"{timestamp}\nMSG,3,0,0,{hex_id},0,{date_str},{time_str},{date_str},{time_str},{callsign},{altitude},,,{lat:.6f},{lon:.6f},,,,,,,\n"
    )
    msg4 = (
        f"{timestamp}\nMSG,4,0,0,{hex_id},0,{date_str},{time_str},{date_str},{time_str},,{altitude},{speed},{bearing:.1f},,,,,,,,\n"
    )
    return msg1 + msg3 + msg4
