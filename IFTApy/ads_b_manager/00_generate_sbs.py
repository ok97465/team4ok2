"""SBS 데이터를 무작위로 생성하는 코드.

@author: ok97465
"""

# %% Imports
# Standard Libraries
from datetime import datetime, timedelta
from random import choice, choices

# Third Party Libraries
from mkl_random import randint, uniform
from numpy import arcsin, arctan2, array, cos, deg2rad, ndarray, rad2deg, sin
from tqdm import tqdm

# %% Parameters
n_aircraft = 2000
duration_min = 1  # 생성할 비행 시간 [minutes]
dt_sec = 0.3  # 비행기 위치를 Update할 간격[sec]
lat_deg_range = [40.4406248 - 0.3, 40.4406248 + 0.3]  # 비행기 위도 범위
lon_deg_range = [-79.9958864 - 0.3, -79.9958864 + 0.3]  # 비행기 경도 범위
alt_m_range = [5000, 40000]  # 비행기 고도 범위
vel_knots_range = [200, 250]  # 비행기 속도 범위

output_file = f"simulated_n({n_aircraft})_dt({dt_sec:.1f}).sbs"


# %% Functions
def generate_hexid(n: int) -> list[str]:
    """항공기가 제작되면 부여되는 고유번호를 생성한다.

    Args:
        n: 생성할 개수.

    """
    return ["".join(choices("0123456789ABCDEF", k=6)) for _ in range(n)]


def generate_callsign(n: int) -> list[str]:
    """항공기 경로에 따라 부여되는 callsign을 생성한다.

    Args:
        n: 생성할 개수.

    """
    prefix_list = ["AAL", "KAL", "DAL", "UAL", "BAW", "JAL"]
    return [choice(prefix_list) + str(randint(100, 9999, 1)) for _ in range(n)]


def calc_next_pos(
    lat_deg: ndarray, lon_deg: ndarray, knots: ndarray, head_deg: ndarray, dt_sec: float
) -> list[ndarray, ndarray]:
    """일정 시간 이후의 비행기 위치를 계산한다.

    Args:
        lat_deg: 현재 위도.
        lon_deg: 현재 경도.
        knots: 비행기 속도[knots].
        head_deg: 비행기 진행방향(yaw)[deg].
        dt_sec: 지연 시간[sec].

    Returns:
        다음 위도, 다음 경도

    """
    R = 6371e3  # 지구 반지름 [m]
    dist_m = knots * 1852 * (dt_sec / 3600)  # 1knots -> 1852 km/h
    head_rad = deg2rad(head_deg)
    lat1 = deg2rad(lat_deg)
    lon1 = deg2rad(lon_deg)
    lat2 = arcsin(
        sin(lat1) * cos(dist_m / R) + cos(lat1) * sin(dist_m / R) * cos(head_rad)
    )
    lon2 = lon1 + arctan2(
        sin(head_rad) * sin(dist_m / R) * cos(lat1),
        cos(dist_m / R) - sin(lat1) * sin(lat2),
    )

    return rad2deg(lat2), rad2deg(lon2)


# %% 비행기 정보 생성
lats_deg = uniform(*lat_deg_range, size=n_aircraft)
lons_deg = uniform(*lon_deg_range, size=n_aircraft)
alts_m = randint(*alt_m_range, size=n_aircraft)
speeds_knots = randint(*vel_knots_range, size=n_aircraft)
heads_deg = randint(0, 360, size=n_aircraft)  # 항공기 진행 방향
hexids = array(generate_hexid(n_aircraft))
callsigns = array(generate_callsign(n_aircraft))

# %% Scripts
now = datetime.utcnow()
n_steps = int(duration_min * 60 / dt_sec)

with open(output_file, "w") as fp:
    for step in tqdm(range(n_steps), ncols=88, desc="Step"):
        current_time = now + timedelta(seconds=dt_sec * step)
        date_str = current_time.strftime("%Y/%m/%d")
        time_str = current_time.strftime("%H:%M:%S.%f")[:-3]
        timestamp = int(current_time.timestamp() * 1000)

        # MSG 1
        lines1 = [
            (
                f"{timestamp}\nMSG,1,0,0,{hx},0,{date_str},{time_str},{date_str},"
                f"{time_str},{cs},,,,,,,,,,,"
            )
            for hx, cs in zip(hexids, callsigns)
        ]
        # MSG 3
        lines3 = [
            (
                f"{timestamp}\nMSG,3,0,0,{hx},0,{date_str},{time_str},{date_str},"
                f"{time_str},{cs},{alt},,,{lat:.6f},{lon:.6f},,,,,,"
            )
            for hx, cs, alt, lat, lon in zip(
                hexids, callsigns, alts_m, lats_deg, lons_deg
            )
        ]
        # MSG 4
        lines4 = [
            (
                f"{timestamp}\nMSG,4,0,0,{hx},0,{date_str},{time_str},{date_str},"
                f"{time_str},,{alt},{spd},{trk},,,,,,,,"
            )
            for hx, alt, spd, trk in zip(hexids, alts_m, speeds_knots, heads_deg)
        ]
        fp.write("\n".join(lines1 + lines3 + lines4) + "\n")

        # 비행 위치 Update
        lats_deg, lons_deg = calc_next_pos(
            lats_deg, lons_deg, speeds_knots, heads_deg, dt_sec
        )

        # TODO: 고도 변화 (필요한가???)
        # alts_m += randint(-50, 51, size=n_aircraft)
