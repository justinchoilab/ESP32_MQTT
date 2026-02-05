from datetime import datetime
import fcntl
import os
import sys
import paho.mqtt.client as mqtt
import psycopg2

def lock_process():
    # 잠금 파일 경로 (시스템 임시 폴더)
    lock_file = "/tmp/esp32_mqtt.lock"
    
    # 파일을 열고 잠금을 시도
    f = open(lock_file, 'w')
    try:
        # LOCK_EX: 배타적 잠금 (다른 프로세스가 접근 불가)
        # LOCK_NB: 비차단 (잠겨있으면 바로 에러 발생)
        fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except IOError:
        print("경고: 이미 서비스나 다른 프로세스가 실행 중입니다. 종료합니다.")
        sys.exit(0)
    
    return f # 변수가 살아있는 동안 잠금이 유지됨

lock_handle = lock_process()

# PostgreSQL 연결 설정
def get_db_connection():
    return psycopg2.connect(
        host=os.getenv("PG_HOST"),
        database=os.getenv("PG_DATABASE"),
        user=os.getenv("PG_USER"),
        password=os.getenv("PG_PASS")
    )

def to_float_or_none(s):
    try:
        return float(s)
    except ValueError:
        return None

# 메시지를 받았을 때 실행될 콜백 함수
def on_message(client, userdata, msg):
    payload = msg.payload.decode("utf-8")
    print(f"Topic: {msg.topic} | Message: {payload}")
    payloadSplit = payload.split(',')
    temperature = to_float_or_none(payloadSplit[0])
    humidity = to_float_or_none(payloadSplit[1])

    try:
        conn = get_db_connection()
        cur = conn.cursor()
        insert_query = "INSERT INTO mqtt.temperature_humidity_logs (temperature, humidity) VALUES (%s, %s)"
        cur.execute(insert_query, (temperature, humidity))
        conn.commit()
        cur.close()
        conn.close()
        last_ins_time = datetime.now()
    except Exception as e:
        print(f"Error inserting to DB: {e}")

# MQTT 클라이언트 설정
client = mqtt.Client()
client.on_message = on_message
client.connect(os.getenv("PG_HOST"), 1883, 60)
client.subscribe("esp32/dht22")
client.loop_forever()