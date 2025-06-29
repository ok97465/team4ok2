"""."""
# %% imports
# Standard Libraries
from typing import Callable

# Third Party Libraries
import matplotlib.pyplot as plt
import requests
from geographiclib.geodesic import Geodesic
from numpy import array, cos, deg2rad, exp
from scipy.interpolate import splev, splrep


class AdvancedFlightPathPlanner:
    """항공기 성능 기반의 비행 경로 생성 및 관리 클래스.

    이 클래스는 출발 및 도착 공항의 ICAO 코드를 받아, 실제 항공기 성능
    파라미터(상승률, 가속도 등)를 기반으로 현실적인 비행 경로를 생성합니다.
    경로는 'Fly-by' 방식으로 Waypoint를 통과하며, 단거리 비행 시 순항 단계가
    없는 프로파일을 자동으로 계산합니다.

    Attributes:
        origin_icao (str): 출발 공항의 ICAO 코드.
        dest_icao (str): 도착 공항의 ICAO 코드.
        path_data (Optional[dict[str, list[float]]]): 생성된 경로 데이터.
            경로 생성이 성공하면 데이터가 채워지고, 실패하면 None이 됩니다.
            {'time': [...], 'lat': [...], ...} 형태의 딕셔너리입니다.
        waypoints (list[tuple[float, float]]): API로부터 가져온 Waypoint 리스트.
    """

    def __init__(
        self,
        origin_icao: str,
        dest_icao: str,
        max_altitude_m: float,
        cruising_speed_mps: float,
        time_step: float,
        smoothing_factor: float = 0.5,
        climb_rate_mps: float = 12.5,
        descent_rate_mps: float = 10.0,
        acceleration_mps2: float = 0.8,
        deceleration_mps2: float = 1.0,
    ) -> None:
        """AdvancedFlightPathPlanner를 초기화하고 비행 경로를 생성합니다.

        Args:
            origin_icao: 출발 공항의 ICAO 코드 (예: "RKSI").
            dest_icao: 도착 공항의 ICAO 코드 (예: "RKPC").
            max_altitude_m: 최대 순항 고도 (미터).
            cruising_speed_mps: 순항 속도 (m/s).
            time_step: 시뮬레이션 시간 간격 (초).
            smoothing_factor: 경로 스무딩 계수. 클수록 경로는 부드러워지지만
                Waypoint에서 더 멀어질 수 있습니다. (기본값: 0.5).
            climb_rate_mps: 평균 초당 상승률 (m/s). (기본값: 12.5, 약 2500 ft/min).
            descent_rate_mps: 평균 초당 강하율 (m/s). (기본값: 10.0, 약 2000 ft/min).
            acceleration_mps2: 평균 가속도 (m/s²). (기본값: 0.8, 약 1.5 knots/sec).
            deceleration_mps2: 평균 감속도 (m/s²). (기본값: 1.0, 약 2.0 knots/sec).
        """
        self.origin_icao = origin_icao
        self.dest_icao = dest_icao
        self.path_data: dict[str, list[float]] | None = None
        self.waypoints: list[tuple[float, float]] = []

        print(f"Fetching waypoints for flight from {origin_icao} to {dest_icao}...")
        self.waypoints = self._get_waypoints_from_flightplandb(origin_icao, dest_icao)

        if not self.waypoints or len(self.waypoints) < 2:
            print("Could not retrieve valid waypoints. Path generation aborted.")
            return

        print(
            f"Successfully fetched {len(self.waypoints)} waypoints. "
            "Generating performance-based flight path..."
        )
        self.path_data = self._generate_smooth_flight_path(
            time_step=time_step,
            max_altitude_m=max_altitude_m,
            cruising_speed_mps=cruising_speed_mps,
            smoothing_factor=smoothing_factor,
            climb_rate_mps=climb_rate_mps,
            descent_rate_mps=descent_rate_mps,
            acceleration_mps2=acceleration_mps2,
            deceleration_mps2=deceleration_mps2,
        )
        print("Flight path generation complete.")

    def _get_waypoints_from_flightplandb(
        self, origin: str, dest: str
    ) -> list[tuple[float, float]]:
        """FlightPlanDatabase API에서 비행 계획의 Waypoint들을 가져옵니다.

        Args:
            origin: 출발 공항의 ICAO 코드.
            dest: 도착 공항의 ICAO 코드.

        Returns:
            (위도, 경도) 튜플의 리스트. 오류 발생 또는 경로 없음 시 빈 리스트 반환.
        """
        try:
            url = (
                "https://api.flightplandatabase.com/search/plans?fromICAO="
                f"{origin}&toICAO={dest}"
            )
            response = requests.get(url)
            response.raise_for_status()
            plans = response.json()
            if not plans:
                return []

            plan_id = plans[0]["id"]
            url_detail = f"https://api.flightplandatabase.com/plan/{plan_id}"
            response_detail = requests.get(url_detail)
            response_detail.raise_for_status()
            plan_detail = response_detail.json()
            nodes = plan_detail.get("route", {}).get("nodes", [])

            wps = [
                (node["lat"], node["lon"])
                for node in nodes
                if node.get("type") in ("VOR", "NDB", "FIX", "LATLON", "APT")
                and node.get("lat") is not None
                and node.get("lon") is not None
            ]
            return wps
        except requests.exceptions.RequestException as e:
            print(f"An API error occurred: {e}")
            return []

    def _calculate_flight_phases(
        self,
        total_distance: float,
        max_altitude: float,
        cruising_speed: float,
        initial_speed: float,
        climb_rate: float,
        descent_rate: float,
        acceleration: float,
        deceleration: float,
    ) -> tuple[float, float]:
        """항공기 성능을 기반으로 상승, 순항, 하강 구간의 시작/종료 거리를 계산합니다.

        Args:
            total_distance: 총 비행 경로 거리 (미터).
            max_altitude: 최대 순항 고도 (미터).
            cruising_speed: 순항 속도 (m/s).
            initial_speed: 초기/최종 속도 (m/s).
            climb_rate: 평균 상승률 (m/s).
            descent_rate: 평균 강하율 (m/s).
            acceleration: 평균 가속도 (m/s²).
            deceleration: 평균 감속도 (m/s²).

        Returns:
            (순항 시작 거리, 순항 종료 거리) 튜플.
            단거리 비행 시 두 값은 동일할 수 있습니다.
        """
        # 1. 상승 구간 계산
        time_to_climb = max_altitude / climb_rate if climb_rate > 0 else 0
        time_to_accelerate = (
            (cruising_speed - initial_speed) / acceleration if acceleration > 0 else 0
        )
        climb_time = max(time_to_climb, time_to_accelerate)
        avg_climb_speed = (initial_speed + cruising_speed) / 2
        climb_dist = avg_climb_speed * climb_time

        # 2. 하강 구간 계산
        time_to_descend = max_altitude / descent_rate if descent_rate > 0 else 0
        time_to_decelerate = (
            (cruising_speed - initial_speed) / deceleration if deceleration > 0 else 0
        )
        descent_time = max(time_to_descend, time_to_decelerate)
        avg_descent_speed = (cruising_speed + initial_speed) / 2
        descent_dist = avg_descent_speed * descent_time

        # 3. 비행 단계 결정
        if climb_dist + descent_dist >= total_distance:
            print("Short flight detected: No cruise phase.")
            # 상승과 하강이 만나는 지점을 비율에 따라 결정
            ratio = climb_dist / (climb_dist + descent_dist)
            cruise_start_dist = total_distance * ratio
            cruise_end_dist = cruise_start_dist
        else:
            cruise_start_dist = climb_dist
            cruise_end_dist = total_distance - descent_dist

        print(
            f"Climb: {climb_dist / 1000:.1f} km, "
            f"Cruise: {(cruise_end_dist - cruise_start_dist) / 1000:.1f} km, "
            f"Descent: {(total_distance - cruise_end_dist) / 1000:.1f} km"
        )
        return cruise_start_dist, cruise_end_dist

    @staticmethod
    def _create_profile_function(
        cruise_start_dist: float,
        cruise_end_dist: float,
        total_distance: float,
        max_value: float,
        min_value: float,
    ) -> Callable[[float], float]:
        """계산된 비행 단계 거리를 기반으로 프로파일 함수(고도 또는 속도)를 생성합니다.

        Args:
            cruise_start_dist: 순항 시작 거리 (미터).
            cruise_end_dist: 순항 종료 거리 (미터).
            total_distance: 총 비행 경로 거리 (미터).
            max_value: 프로파일의 최대값 (예: 최대 고도).
            min_value: 프로파일의 최소값 (예: 초기 속도).

        Returns:
            거리를 인자로 받아 해당 지점의 프로파일 값을 반환하는 함수.
        """
        k_up = 10.0 / cruise_start_dist if cruise_start_dist > 0 else float("inf")
        descent_dist = total_distance - cruise_end_dist
        k_down = 10.0 / descent_dist if descent_dist > 0 else float("inf")

        def profile_function(d: float) -> float:
            if d < cruise_start_dist:
                return min_value + (max_value - min_value) * (
                    1 / (1 + exp(-k_up * (d - cruise_start_dist / 2)))
                )
            elif d < cruise_end_dist:
                return max_value
            else:
                return min_value + (max_value - min_value) * (
                    1 / (1 + exp(k_down * (d - cruise_end_dist - descent_dist / 2)))
                )

        return profile_function

    def _generate_smooth_flight_path(
        self,
        time_step: float,
        max_altitude_m: float,
        cruising_speed_mps: float,
        smoothing_factor: float,
        climb_rate_mps: float,
        descent_rate_mps: float,
        acceleration_mps2: float,
        deceleration_mps2: float,
    ) -> dict[str, list[float]]:
        """성능 모델을 기반으로 부드러운 전체 비행 경로 데이터를 생성합니다.

        이 함수는 클래스의 핵심 로직으로, Waypoint 기반의 B-Spline 경로를 생성하고,
        항공기 성능 파라미터를 이용해 비행 단계를 계산한 뒤, 각 시간 스텝별로
        위치, 고도, 속도, 기수 정보를 포함하는 완전한 궤적을 만듭니다.

        Args:
            (모든 파라미터는 __init__에서 전달됨)

        Returns:
            생성된 경로 데이터가 포함된 딕셔너리.
        """
        geod = Geodesic.WGS84
        lats = array([wp[0] for wp in self.waypoints])
        lons = array([wp[1] for wp in self.waypoints])
        initial_speed = 10.0

        cumulative_distances = [0.0]
        total_distance = 0.0
        for i in range(len(self.waypoints) - 1):
            g = geod.Inverse(lats[i], lons[i], lats[i + 1], lons[i + 1])
            total_distance += g["s12"]
            cumulative_distances.append(total_distance)

        lat_spline_rep = splrep(cumulative_distances, lats, s=smoothing_factor)
        lon_spline_rep = splrep(cumulative_distances, lons, s=smoothing_factor)

        cruise_start_dist, cruise_end_dist = self._calculate_flight_phases(
            total_distance,
            max_altitude_m,
            cruising_speed_mps,
            initial_speed,
            climb_rate_mps,
            descent_rate_mps,
            acceleration_mps2,
            deceleration_mps2,
        )

        altitude_profile = self._create_profile_function(
            cruise_start_dist, cruise_end_dist, total_distance, max_altitude_m, 0
        )
        speed_profile = self._create_profile_function(
            cruise_start_dist,
            cruise_end_dist,
            total_distance,
            cruising_speed_mps,
            initial_speed,
        )

        path_points = []
        current_dist, current_time = 0.0, 0.0
        while current_dist < total_distance:
            speed = speed_profile(current_dist)
            path_points.append(
                {
                    "time": current_time,
                    "lat": float(splev(current_dist, lat_spline_rep)),
                    "lon": float(splev(current_dist, lon_spline_rep)),
                    "alt": altitude_profile(current_dist),
                    "speed": speed,
                }
            )
            delta_dist = speed * time_step
            current_dist += delta_dist
            current_time += time_step

        path_points.append(
            {
                "time": current_time,
                "lat": lats[-1],
                "lon": lons[-1],
                "alt": 0,
                "speed": initial_speed,
            }
        )

        for i in range(len(path_points) - 1):
            p1 = path_points[i]
            p2 = path_points[i + 1]
            g = geod.Inverse(p1["lat"], p1["lon"], p2["lat"], p2["lon"])
            p1["heading"] = (g["azi1"] + 360) % 360

        if len(path_points) > 1:
            path_points[-1]["heading"] = path_points[-2]["heading"]
        else:
            path_points[-1]["heading"] = 0

        return {key: [p[key] for p in path_points] for key in path_points[0]}

    def plot_results(self) -> None:
        """생성된 비행 경로 데이터를 다양한 그래프로 시각화합니다."""
        if not self.path_data:
            print("No path data to plot.")
            return

        fig, axs = plt.subplots(2, 3, figsize=(18, 12))
        axs = axs.ravel()
        fig.suptitle(
            f"Flight Path Analysis: {self.origin_icao} to {self.dest_icao}",
            fontsize=16,
        )

        axs[0].plot(self.path_data["time"], self.path_data["lat"])
        axs[0].set_title("Latitude vs Time")
        axs[0].grid(True)

        axs[1].plot(self.path_data["time"], self.path_data["lon"])
        axs[1].set_title("Longitude vs Time")
        axs[1].grid(True)

        axs[2].plot(self.path_data["time"], self.path_data["alt"])
        axs[2].set_title("Altitude vs Time")
        axs[2].grid(True)

        axs[3].plot(self.path_data["time"], self.path_data["speed"], color="green")
        axs[3].set_title("Speed vs Time")
        axs[3].grid(True)

        axs[4].plot(self.path_data["time"], self.path_data["heading"], color="purple")
        axs[4].set_title("Heading vs Time")
        axs[4].grid(True)

        self._plot_2d_path(axs[5])
        plt.tight_layout(rect=[0, 0.03, 1, 0.95])
        plt.show()

    def _plot_2d_path(self, ax: plt.Axes) -> None:
        """2D 비행 경로, Waypoint, 허용 오차 범위를 주어진 축에 그립니다.

        Args:
            ax: 그래프를 그릴 Matplotlib의 축(Axes) 객체.
        """
        ax.plot(
            self.path_data["lon"],
            self.path_data["lat"],
            label="Performance-based Path",
            color="dodgerblue",
            linewidth=2,
            zorder=10,
        )
        if self.waypoints:
            wp_lats, wp_lons = zip(*self.waypoints)
            ax.plot(
                wp_lons,
                wp_lats,
                "o-",
                color="orangered",
                markersize=5,
                label="Waypoints",
                zorder=5,
            )
            radius_in_deg = 3.0 / 111.0
            # 범례에 중복된 라벨이 생기지 않도록 처리
            tolerance_label_added = False
            for lat, lon in self.waypoints:
                lon_radius = radius_in_deg / cos(deg2rad(lat))
                label = "3km Tolerance" if not tolerance_label_added else ""
                circle = plt.Circle(
                    (lon, lat),
                    lon_radius,
                    color="gray",
                    alpha=0.3,
                    zorder=1,
                    label=label,
                )
                ax.add_patch(circle)
                tolerance_label_added = True

        ax.set_title("2D Path with Waypoint Tolerance")
        ax.legend()
        ax.grid(True)


# --- 실행 예제 ---
if __name__ == "__main__":
    planner = AdvancedFlightPathPlanner(
        origin_icao="RKSI",
        dest_icao="RKPC",
        max_altitude_m=9000.0,
        cruising_speed_mps=220.0,
        time_step=20,
        smoothing_factor=0.5,
        climb_rate_mps=12.5,
        descent_rate_mps=10.0,
        acceleration_mps2=0.8,
        deceleration_mps2=1.0,
    )

    if planner.path_data:
        planner.plot_results()
