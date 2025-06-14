# IFTApy
---

## 요구사항
- python > 3.10
- requirements.txt

---

## SBS simulation data 생성
- snippets/00_generate_sbs.py 수행
- Parameter 변경을 위해서는 IFTApy/snippets/00_generate_sbs.py의 Parameters Section 변경

```python
# %% Parameters
n_aircraft = 2000
duration_min = 30  # 생성할 비행 시간 [minutes]
dt_sec = 5  # 비행기 위치를 Update할 간격[sec]
lat_deg_range = [40.4406248 - 0.3, 40.4406248 + 0.3]  # 비행기 위도 범위
lon_deg_range = [-79.9958864 - 0.3, -79.9958864 + 0.3]  # 비행기 경도 범위
alt_m_range = [5000, 40000]  # 비행기 고도 범위
vel_knots_range = [200, 250]  # 비행기 속도 범위

output_file = "simulated.sbs"
```
