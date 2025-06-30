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
UPDATE `scs-lg-arch-4.SBS_DATA_T4.SBS_DATA_TABLE_T4`
SET
  -- 23000.0	41.317291	-84.594543  
  --89.836395	-178.313248
  --269.436951	178.653488
  --40000.0	-25.678028	-58.637608	E0B04E
  --37000.0	269.452698	147.576599	4CA24C
  Altitude = 300000,
  Latitude = -100,
  Longitude = -100
WHERE
  -- 특정 HexIdent와 타임스탬프를 조건으로 지정
  HexIdent = '4CA24C'
  AND TIMESTAMP(DATETIME(Date_MSG_Generated, Time_MSG_Generated), 'UTC') = TIMESTAMP('2025-06-29 14:38:30 UTC')
;

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