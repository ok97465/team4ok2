'''
필요 라이브러리:
pip install google-cloud-bigquery

'''

import os
import sys
import csv
from google.cloud import bigquery

global_filepath = "./BigQuery/"
output_filename = "update.csv"
user_query = """
MERGE INTO
  `scs-lg-arch-4.SBS_DATA_T4.SBS_DATA_TABLE_T4` AS T  -- 업데이트할 대상 테이블
USING
  (
    -- 이 부분은 사용자가 제공한 쿼리입니다.
    -- 여기서 업데이트 대상이 되는 HexIdent와 timestamp_utc를 식별합니다.
    WITH PredictionResults AS (
      SELECT
        DISTINCT pred.* EXCEPT(nearest_centroids_distance),
        pred.nearest_centroids_distance[OFFSET(0)].DISTANCE AS distance_from_centroid
      FROM
        ML.PREDICT(
          MODEL `scs-lg-arch-4.SBS_DATA_T4.altitude_kmeans_model`,
          (
            SELECT
              SAFE_CAST(Altitude AS FLOAT64) AS Altitude,
              SAFE_CAST(Latitude AS FLOAT64) AS Latitude,
              SAFE_CAST(Longitude AS FLOAT64) AS Longitude,
              HexIdent,
              TIMESTAMP(DATETIME(Date_MSG_Generated, Time_MSG_Generated), 'UTC') AS timestamp_utc
            FROM
              `scs-lg-arch-4.SBS_DATA_T4.SBS_DATA_TABLE_T4`
            WHERE
              TIMESTAMP(DATETIME(Date_MSG_Generated, Time_MSG_Generated), 'UTC') >= TIMESTAMP_SUB(CURRENT_TIMESTAMP(), INTERVAL 1 HOUR)
              AND SAFE_CAST(Altitude AS FLOAT64) IS NOT NULL AND Altitude > 0
          )
        ) AS pred
    )
    SELECT
      HexIdent,
      timestamp_utc
    FROM
      PredictionResults
--    WHERE
--    distance_from_centroid >= 20
    ORDER BY
      distance_from_centroid DESC
    LIMIT 6
  ) AS S  -- 업데이트할 데이터 소스
ON
  T.HexIdent = S.HexIdent
  AND TIMESTAMP(DATETIME(T.Date_MSG_Generated, T.Time_MSG_Generated), 'UTC') = S.timestamp_utc
WHEN MATCHED THEN
  UPDATE SET
    Altitude = 10000,   -- <-- 이 부분을 원하는 값으로 변경하세요 (예: 0 또는 -9999).
    Latitude = 11,   -- <-- 이 부분을 원하는 값으로 변경하세요.
    Longitude = 11;   -- <-- 이 부분을 원하는 값으로 변경하세요.
"""
# 인증 json 파일 경로 (업로드용과 동일 이름 가정)
api_key = os.path.join(global_filepath, "YourJsonFile.json")
os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = api_key

# BigQuery Client 생성
client = bigquery.Client()

try:
    query_job = client.query(user_query)
    results = query_job.result()

    # csv로 저장
    print(f"BigQuery updated.")
except Exception as e:
    print("Error downloading data from BigQuery:", e)