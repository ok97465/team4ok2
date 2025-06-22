'''
필요 라이브러리:
pip install google-cloud-bigquery
'''

import os
import sys
from google.cloud import bigquery

if len(sys.argv) == 2:
    global_filepath = sys.argv[1]
    print(f"The first argument is: {global_filepath}")
else:
    print("Usage: python SimpleBigQueryTruncate.py [json파일경로포함폴더/]")
    sys.exit(1)

# 인증 json 파일 경로 (업로드용과 동일 이름 가정)
api_key = os.path.join(global_filepath, "YourJsonFile.json")
os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = api_key

# BigQuery Client 생성
client = bigquery.Client()

# 삭제할 테이블 ID (업로드용과 동일하게 지정)
table_id = "scs-lg-arch-4.SBS_DATA_T4.SBS_DATA_TABLE_T4"

# TRUNCATE 쿼리 실행 (테이블 스키마는 유지, 데이터만 삭제)
query = f"TRUNCATE TABLE `{table_id}`"
print("Query:", query)

try:
    query_job = client.query(query)
    query_job.result()  # Wait for completion
    print(f"테이블 {table_id}의 모든 데이터가 삭제(Truncate) 되었습니다.")
except Exception as e:
    print("Error truncating table:", e)