#!/usr/bin/env python3
import json
from kafka import KafkaConsumer
from influxdb_client import InfluxDBClient, Point, WritePrecision

# --- Kafka consumer ---
consumer = KafkaConsumer(
    'metrics',
    bootstrap_servers='localhost:9092',
    value_deserializer=lambda m: json.loads(m.decode())
)

# --- InfluxDB client ---
client = InfluxDBClient(
    url="http://localhost:8086",
    token="admin:admin",
    org="myorg"
)
# Just use the default write_api (batching); no WritePrecision here
write_api = client.write_api()

print("Consuming metrics → writing to InfluxDB…")
for msg in consumer:
    m = msg.value
    # Build a point with nanosecond timestamps
    p = Point(m["metric"]) \
        .field("value", m["value"]) \
        .tag("symbol", m.get("symbol", "")) \
        .time(m["timestamp"], WritePrecision.NS)
    write_api.write(bucket="metrics", record=p)
