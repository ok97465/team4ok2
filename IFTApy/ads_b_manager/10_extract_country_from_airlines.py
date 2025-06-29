"""Airlines 정보에서 IATA와 ICAO의 국적을 읽어온다.

https://raw.githubusercontent.com/jpatokal/openflights/master/data/airlines.dat에서
Airlines정보를 다운받아. IATA와 ICAO와 국적의 Mapping 정보를 가져온다.

Created on Sat Jun 28 13:20:42 2025

@author: ok974

"""

# %% Imports
# Standard Libraries
import csv
import re

# Third Party Libraries
import pandas as pd

path_data = "../AircraftDB/airlines.csv"
path_out = "../AircraftDB/airline_names_countries.csv"

df = pd.read_csv(path_data)

airlines_infos = dict()
pattern_code = r"^[A-Za-z0-9]+$"
pattern_country = r"^[A-Za-z0-9 ]+$"

for index, row in df.iterrows():
    if (
        pd.isna(row["Country"])
        or bool(re.match(pattern_country, row["Country"])) is False
    ):
        continue

    if (
        row["IATA"] == "-"
        or pd.isna(row["IATA"])
        or bool(re.match(pattern_code, row["IATA"])) is False
    ):
        continue

    airlines_infos[row["IATA"]] = (row["Country"], row["Name"])
    if (
        row["ICAO"] == "-"
        or pd.isna(row["ICAO"])
        or bool(re.match(pattern_code, row["ICAO"])) is False
    ):
        continue

    airlines_infos[row["ICAO"]] = (row["Country"], row["Name"])

# CSV로 저장
with open(path_out, "w", newline="", encoding="utf-8") as csvfile:
    writer = csv.writer(csvfile)
    # 헤더 작성
    writer.writerow(["AirlineCode", "Country", "AirlineName"])
    # 데이터 작성
    for code, (country, airline_name) in airlines_infos.items():
        writer.writerow([code, country, airline_name])

print(f"저장 완료: {path_out}")
