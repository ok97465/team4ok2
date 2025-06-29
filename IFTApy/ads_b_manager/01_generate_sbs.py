"""ADS-B 데이터를 시뮬레이션하고 SBS-1 형식으로 저장하는 스크립트.

이 스크립트는 'routes.csv' 파일에서 실제 항공편 경로를 읽어와,
'airports.csv' 파일의 공항 위치 데이터를 기반으로 항공기의 움직임을 시뮬레이션합니다.

주요 기능:
- 'routes.csv'에서 무작위로 항공편을 선택합니다.
- 선택된 항공편의 출발 공항에서 시뮬레이션을 시작합니다.
- 도착 공항을 향해 일정한 속도와 고도로 비행합니다.
- 시뮬레이션 데이터는 'MSG,1', 'MSG,3', 'MSG,4' 세 가지 유형의
  SBS-1 메시지 형식으로 생성되어 파일에 저장됩니다.
- 생성되는 파일 이름은 시뮬레이션 조건(항공기 수, 시간 간격)을 포함합니다.
"""

# %% Imports
# Standard Libraries
import os.path as osp
from datetime import datetime, timedelta
from random import choice, randint, uniform

# Third Party Libraries
from geographiclib.geodesic import Geodesic
from tqdm import tqdm

# Project Libraries
from ads_b_manager import (
    calculate_distance_nm,
    calculate_initial_bearing,
    calculate_next_position,
    format_sbs_messages,
    load_data,
    load_icao24_list,
)

# --- 시뮬레이션 설정 ---
# 이 스크립트가 있는 폴더를 기준으로 상대 경로 설정
ROUTES_CSV = osp.join("..", "AircraftDB", "routes.csv")
AIRPORTS_CSV = osp.join("..", "AircraftDB", "airports.csv")
ICAO24_CSV = osp.join("..", "AircraftDB", "aircraftDatabase.csv")


N_AIRCRAFT = 2000  # 시뮬레이션할 항공기 수
DURATION_MIN = 1  # 각 항공기의 비행 시뮬레이션 시간 (분)
DT_SEC = 0.5  # 데이터 생성 시간 간격 (초)
ALT_FT_RANGE = [15000, 40000]  # 고도 범위 (피트)
VEL_KNOTS_RANGE = [100, 500]  # 속도 범위 (노트)


# 데이터 로드
routes = load_data(ROUTES_CSV)
airports = load_data(AIRPORTS_CSV)
icao24_list = load_icao24_list(ICAO24_CSV)

if not routes or not airports:
    raise RuntimeError("데이터 로딩 실패. 스크립트를 종료합니다.")

# --- 유효한 route만 미리 필터링 ---
valid_routes = [
    r
    for r in routes
    if (
        r.get("Callsign", "").strip()
        and len(r.get("AirportCodes", "").split("-")) >= 2
        and r.get("AirportCodes", "").split("-")[0] in airports
        and r.get("AirportCodes", "").split("-")[-1] in airports
    )
]
if not valid_routes:
    raise RuntimeError("유효한 route가 없습니다. 스크립트를 종료합니다.")


aircraft_data = []
geod = Geodesic.WGS84
for _ in range(N_AIRCRAFT):
    route_info = choice(valid_routes)
    callsign = route_info.get("Callsign", "").strip()
    airport_codes = route_info.get("AirportCodes", "").split("-")
    dep_icao = airport_codes[0]
    arr_icao = airport_codes[-1]

    dep_lat, dep_lon = airports[dep_icao]
    dest_lat, dest_lon = airports[arr_icao]

    # 출발-도착 geodesic 선 위 임의의 위치에서 시작 (0~1 사이 비율)
    start_fraction = uniform(0, 1)
    inv = geod.Inverse(dep_lat, dep_lon, dest_lat, dest_lon)
    total_dist = inv["s12"]
    dist_along = total_dist * start_fraction
    direct = geod.Direct(dep_lat, dep_lon, inv["azi1"], dist_along)
    start_lat, start_lon = direct["lat2"], direct["lon2"]

    aircraft_data.append(
        {
            "hex_id": (
                choice(icao24_list)
                if icao24_list
                else f"{randint(0, 0xFFFFFF):06X}"
            ),
            "callsign": callsign,
            "lat": start_lat,
            "lon": start_lon,
            "dest_lat": dest_lat,
            "dest_lon": dest_lon,
            "altitude": randint(*ALT_FT_RANGE),
            "speed": randint(*VEL_KNOTS_RANGE),
            "bearing": calculate_initial_bearing(start_lat, start_lon, dest_lat, dest_lon),
            "active": True,  # 비행기가 활성 상태인지 추적
        }
    )

# 출력 파일 이름 설정
output_file = osp.join(
    "..", f"simulated_routes_n({N_AIRCRAFT})_dt({DT_SEC:.1f}).sbs"
)

# --- 시간 단계별 시뮬레이션 루프 ---
with open(output_file, "w", encoding="utf-8") as f:
    n_steps = int(DURATION_MIN * 60 / DT_SEC)
    start_time = datetime.utcnow()

    for step in tqdm(range(n_steps), desc="Simulating Timesteps"):
        current_time = start_time + timedelta(seconds=step * DT_SEC)
        date_str = current_time.strftime("%Y/%m/%d")
        time_str = current_time.strftime("%H:%M:%S.%f")[:-3]
        timestamp = int(current_time.timestamp() * 1000)

        # 모든 항공기에 대해 메시지 생성 및 위치 업데이트
        for ac in aircraft_data:
            # 비활성 항공기는 건너뜀
            if not ac["active"]:
                continue

            # 공통 포맷 함수 사용
            f.write(format_sbs_messages(timestamp, date_str, time_str, ac))

            # 다음 위치 계산 및 업데이트
            distance_nm = ac["speed"] * (DT_SEC / 3600)
            next_lat, next_lon = calculate_next_position(
                ac["lat"], ac["lon"], ac["bearing"], distance_nm
            )
            ac["lat"] = next_lat
            ac["lon"] = next_lon

            # 목적지 도착 여부 확인
            remaining_distance = calculate_distance_nm(
                next_lat, next_lon, ac["dest_lat"], ac["dest_lon"]
            )
            if remaining_distance < 10:  # 10 해리 이내로 들어오면 도착으로 간주
                ac["active"] = False

print(f"\n✅ SBS data generation complete. Saved to '{output_file}'")
