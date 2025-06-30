import os
import winsound
import sys
import csv
from google.cloud import bigquery
from google.api_core import retry

# --- Configuration for Upload ---
DESTINATION_TABLE_ID = "scs-lg-arch-4.SBS_DATA_T4.SBS_DATA_TABLE_T4"

# --- Configuration for Query Result ---
output_filename = "result.csv"
user_query = """
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
  *
FROM
  PredictionResults
WHERE
  distance_from_centroid >= 20 -- 여기에서 WHERE 절을 사용하여 필터링합니다.
ORDER BY
  distance_from_centroid DESC
LIMIT 10;
"""

# --- Function to load a single CSV file ---
@retry.Retry(deadline=600.0)
def load_csv_file_to_bigquery(filename, client, global_filepath):
    """
    Reads a CSV file in binary mode and loads it to a BigQuery table as-is.
    """
    try:
        print(f"Reading and loading file: {filename}", flush=True)
        
        # Use 'rb' mode for binary upload, as required by the BigQuery client
        with open(os.path.join(global_filepath, filename), "rb") as source_file:
            # Set up the load job configuration
            job_config = bigquery.LoadJobConfig(
                source_format=bigquery.SourceFormat.CSV,
                autodetect=True,
                skip_leading_rows=1,
                write_disposition=bigquery.WriteDisposition.WRITE_APPEND,
            )
            # Start the load job from the file
            job = client.load_table_from_file(
                source_file,
                DESTINATION_TABLE_ID,
                job_config=job_config
            )
            
        # This will wait for the BigQuery job to complete and check for errors
        job.result(timeout=600)
        print(f"Successfully loaded {job.output_rows} rows to {DESTINATION_TABLE_ID}.", flush=True)
        
        # Delete the original file after successful upload
        if os.path.exists(os.path.join(global_filepath, filename)):
           os.remove(os.path.join(global_filepath, filename))
           print(f"Original file '{filename}' deleted successfully.", flush=True)
        
        return 0

    except FileNotFoundError:
        print(f"Error: File '{os.path.join(global_filepath, filename)}' not found.", flush=True)
        return 1
    except Exception as e:
        print(f"An unexpected error occurred: {e}", flush=True)
        return 1
            
# --- Main execution part ---
if __name__ == "__main__":
    if len(sys.argv) == 3:
        global_filepath = sys.argv[1]
        filename = sys.argv[2]
        print(f"Argument 1 (global_filepath): {global_filepath}", flush=True)
        print(f"Argument 2 (filename): {filename}", flush=True)
    else:
        print(f"Usage: python your_script.py <filepath> <filename>")
        print(f"Failure 1: Missing arguments.", flush=True)
        sys.exit(1)

    # Set credentials
    api_key_path = os.path.join(global_filepath, "YourJsonFile.json") 
    if not os.path.exists(api_key_path):
        print(f"Error: Service account key key not found at {api_key_path}. Please check the path.", flush=True)
        sys.exit(1)
    os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = api_key_path

    # Construct a BigQuery client object.
    client = bigquery.Client()

    # --- Step 1: Upload the CSV file ---
    upload_result = load_csv_file_to_bigquery(filename, client, global_filepath)

    # --- Step 2: If upload is successful, run the query and save results ---
    if upload_result == 0:
        print("\n--- Upload successful. Running detection query ---", flush=True)
        try:
            query_job = client.query(user_query)
            results = query_job.result()

            # Save the query result to a CSV file
            output_path = os.path.join(global_filepath, output_filename)
            with open(output_path, 'w', newline='', encoding='utf-8-sig') as csvfile:
                writer = csv.writer(csvfile)
                writer.writerow([field.name for field in results.schema])
                for row in results:
                    writer.writerow(list(row))

            print(f"BigQuery query results successfully downloaded and saved to '{output_path}'.", flush=True)
            print(f"Success\n", flush=True)
            # Play a beep sound on Windows for success notification
            if sys.platform == "win32":
                try:
                    winsound.Beep(2500, 1000)
                except Exception as e:
                    pass
        except Exception as e:
            print("Error downloading data from BigQuery:", e, flush=True)
            print(f"Failure 2: Query execution failed.\n", flush=True)
            sys.exit(1)
    else:
        print(f"Failure 2: Upload failed.\n", flush=True)
        sys.exit(1)