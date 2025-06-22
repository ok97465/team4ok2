'''
필요 라이브러리:
pip install google-cloud-bigquery
'''

import os
import sys
import csv
from google.cloud import bigquery

if len(sys.argv) == 5:
    global_filepath = sys.argv[1]
    output_filename = sys.argv[2]
    filter_date = sys.argv[3]      # YYYY-MM-DD
    filter_time = sys.argv[4]      # HH:MM:SS
    print(f"The first argument is: {global_filepath}")
    print(f"The second argument is: {output_filename}")
    print(f"Filter Date: {filter_date}")
    print(f"Filter Time: {filter_time}")
else:
    print("Usage: python SimpleBigQueryToCSV.py [json파일경로포함폴더/] [내보낼파일명.csv] [YYYY-MM-DD] [HH:MM:SS]")
    sys.exit(1)

current_directory = os.getcwd()
print("Current directory:", current_directory)

# 인증 json 파일 경로 (업로드용과 동일 이름 가정)
api_key = os.path.join(global_filepath, "YourJsonFile.json")
os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = api_key

# BigQuery Client 생성
client = bigquery.Client()

# 다운로드할 테이블 ID (업로드용과 동일하게 지정)
table_id = "scs-lg-arch-4.SBS_DATA_T4.SBS_DATA_TABLE_T4"

# 쿼리: 지정한 날짜/시간 이후 데이터 50000개까지
query = f"""
SELECT *
FROM `{table_id}`
WHERE
  (Date_MSG_Logged > '{filter_date}')
  OR (Date_MSG_Logged = '{filter_date}' AND Time_MSG_Logged > '{filter_time}')
ORDER BY Date_MSG_Logged, Time_MSG_Logged
LIMIT 50000
"""
print("Query:", query)

try:
    query_job = client.query(query)
    results = query_job.result()

    # csv로 저장
    output_path = os.path.join(global_filepath, output_filename)
    with open(output_path, 'w', newline='', encoding='utf-8-sig') as csvfile:
        writer = csv.writer(csvfile)
        # 첫 줄: 컬럼명
        writer.writerow([field.name for field in results.schema])
        # 데이터
        for row in results:
            writer.writerow(list(row))

    print(f"BigQuery 테이블에서 데이터를 성공적으로 다운로드하여 '{output_path}'에 저장했습니다.")
except Exception as e:
    print("Error downloading data from BigQuery:", e)