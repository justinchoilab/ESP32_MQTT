#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 2  // Digital pin connected to
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const char *ssid = "********";
const char *password = "********";

WiFiClient espClient;
PubSubClient client(espClient);

// MQTT 브로커 설정
const char *mqtt_server = "***.***.***.***";
const int mqtt_port = 1883;

const int PUBLISH_INTERVAL_COUNT = 120;      // 5초 * 120 = 10분
const unsigned long SENSOR_INTERVAL = 5000;  // 5초
unsigned long lastTime = 0;
int count = PUBLISH_INTERVAL_COUNT;  // 최초 실행 시 바로 발행되도록 초기화

void setup() {
  Serial.begin(9600);
  dht.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  delay(2000);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();

  if (now - lastTime > SENSOR_INTERVAL) {
    lastTime = now;
    String t = readDHTTemperature();
    String h = readDHTHumidity();

    // OLED 표시
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("T: " + t);
    display.println("RH: " + h);
    display.display();

    // MQTT 발행 (카운트 체크)
    if (count >= PUBLISH_INTERVAL_COUNT) {
      String msg = t + "," + h;
      client.publish("esp32/dht22", msg.c_str());
      Serial.println("MQTT Published: " + msg);
      count = 0;  // 발행 후 리셋
    }

    count++;
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Publisher")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

String readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its averyslow sensor)
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  if (isnan(t)) {
    return "--";
  } else {
    return String(t);
  }
}

String readDHTHumidity() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    return "--";
  } else {
    return String(h);
  }
}
