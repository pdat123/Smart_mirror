
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"

const char* ssid = "Vietien";
const char* password = "26042004";

#define Ledwifi 2
#define RESET_BUTTON 0  // chân bạn nối nút
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;  // UTC+7
const int daylightOffset_sec = 0;

String weatherApiKey = "e2029e14dfbb872bf7a67b3b8b03a3c5";
String weatherUrl = "http://api.openweathermap.org/data/2.5/weather?q=Hanoi&appid=" + weatherApiKey + "&units=metric";

void connectwifi(){
  WiFi.begin(ssid, password);
  unsigned long waittime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - waittime <= 5000)
  {
    Serial.print(".");
    delay(1000);
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }else
  {
    Serial.println("Failed to connect to WiFi");
  }  
}

void setup() {
  Serial.begin(115200);
  pinMode(RESET_BUTTON, INPUT_PULLUP);  // dùng điện trở kéo lên nội bộ
  pinMode(Ledwifi , OUTPUT);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);  // ESP32 TX=17 → STM32 RX

  Serial.print("Connect WiFi");
  connectwifi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {

  if (digitalRead(RESET_BUTTON) == LOW) {  // nút được nhấn (nối xuống GND)
    Serial.println("Reset button pressed. Restarting...");
    delay(100);  // tránh nhiễu
    ESP.restart();  // reset phần mềm
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Reconnecting...");
    connectwifi();
    delay(1000);
    return;
  }
  
  // Lấy thời gian thực
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  char timeStr[32];
  strftime(timeStr, sizeof(timeStr), "%d/%m/%Y,%H:%M", &timeinfo);

  String temp = "N/A";
  String humi = "N/A";

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(weatherUrl);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println("Weather data received");

      // Parse JSON
      const size_t capacity = 1024;
      DynamicJsonDocument doc(capacity);
      deserializeJson(doc, payload);

      float temperature = doc["main"]["temp"];
      int humidity = doc["main"]["humidity"];

      // Định dạng 2 chữ số thập phân
      temp = String(temperature, 2);  // TEMPxx.xx
      humi = String((float)humidity, 2);  // HUMIxx.xx
    } else {
      Serial.print("HTTP Error: ");
      Serial.println(httpCode);
    }
    http.end();
  }

  if (temp == "N/A" && humi == "N/A")
  {
    digitalWrite(Ledwifi, LOW);
  }else
  {
    digitalWrite(Ledwifi, HIGH);
  }
  
  String sendData = String(timeStr) + "," + temp + "," + humi + "\n";
  Serial2.print(sendData);
  Serial.print("Sent to STM32: ");
  Serial.print(sendData);

  delay(5000);  // 5 giây
}

