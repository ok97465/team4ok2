'''
필요 라이브러리:
pip install google-cloud-bigquery

예제>
python ./BigQuery/SimpleBigQueryQueryToCSV.py ./BigQuery/ result.csv "SELECT Callsign FROM `scs-lg-arch-4.SBS_DATA_T4.SBS_DATA_TABLE_T4` LIMIT 100"
'''

import os
import sys
import csv
from google.cloud import bigquery

if len(sys.argv) == 4:
    global_filepath = sys.argv[1]
    output_filename = sys.argv[2]
    user_query = sys.argv[3]
    print(f"The first argument is: {global_filepath}")
    print(f"The second argument is: {output_filename}")
    print(f"Query: {user_query}")
else:
    print("Usage: python SimpleBigQueryQueryToCSV.py [json파일경로포함폴더/] [내보낼파일명.csv] \"[쿼리]\"")
    sys.exit(1)

# 인증 json 파일 경로 (업로드용과 동일 이름 가정)
api_key = os.path.join(global_filepath, "YourJsonFile.json")
os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = api_key

# BigQuery Client 생성
client = bigquery.Client()

try:
    query_job = client.query(user_query)
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

    print(f"BigQuery 쿼리 결과를 성공적으로 다운로드하여 '{output_path}'에 저장했습니다.")
except Exception as e:
    print("Error downloading data from BigQuery:", e)