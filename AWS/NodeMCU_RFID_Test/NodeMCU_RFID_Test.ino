#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_PN532.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

// OLED 핀 (D5 -> GPIO14, D6 -> GPIO12)
#define OLED_SDA 14
#define OLED_SCL 12

// PN532 핀 (D2 -> GPIO4, D1 -> GPIO5)
#define PN532_IRQ   4
#define PN532_RESET 5

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// 부저 핀 (D7 -> GPIO13). 명시적으로 D7 상수를 사용합니다.
#define BUZZER_PIN D7

const char* ssid = "imfall";
const char* password = "00000000";
const char* serverUrl = "http://13.125.123.251/api.php?action=tag";

// 한국 시간 설정 (UTC+9)
const int timezone = 9 * 3600;
const int dst = 0;

bool nfc_ok = false;

void setup() {
  Serial.begin(115200);
  delay(10);
  
  pinMode(BUZZER_PIN, OUTPUT);
  
  // 아두이노 부팅 시 부저가 정상 작동하는지 확인하기 위해 0.1초 동안 삐 소리를 냅니다.
  tone(BUZZER_PIN, 2000, 100);
  
  // I2C 핀 초기화
  Wire.begin(OLED_SDA, OLED_SCL);
  
  // OLED 디스플레이 초기화
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("System Initializing...");
  display.display();
  
  // PN532 초기화
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    display.println("RFID Reader: ERROR");
    display.display();
    delay(2000);
    nfc_ok = false;
  } else {
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
    display.println("RFID Reader: OK");
    display.display();
    nfc.SAMConfig();
    nfc_ok = true;
  }
  
  // WiFi 연결
  display.println("Connecting to WiFi...");
  display.display();
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  display.println("WiFi Connected!");
  display.display();
  
  // NTP 동기화 설정
  configTime(timezone, dst, "pool.ntp.org", "time.nist.gov");
  
  // 유효 시간 수신까지 대기
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    now = time(nullptr);
  }
  
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("System Ready!");
  display.display();
  delay(1000);
}

void loop() {
  // 1. 시간 표시
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  
  display.clearDisplay();
  
  // 연결 상태 표시
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("WiFi: OK");
  
  // 날짜 표시
  display.setCursor(70, 0);
  display.printf("%02d/%02d", timeinfo->tm_mon + 1, timeinfo->tm_mday);
  
  // 시계 표시 (크게)
  display.setTextSize(2);
  display.setCursor(18, 20);
  display.printf("%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  
  // 대기 상태 텍스트
  display.setTextSize(1);
  display.setCursor(18, 48);
  if (nfc_ok) {
    display.print("Tag your RFID Card");
  } else {
    display.print("RFID ERROR: CHECK HW");
  }
  
  display.display();
  
  // 2. RFID 카드 감지 체크 (50ms 타임아웃 비블로킹 방식)
  if (nfc_ok) {
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;
    
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50);
    
    if (success) {
      handleRfidTag(uid, uidLength);
    }
  }
  
  delay(150);
}

void handleRfidTag(uint8_t *uid, uint8_t uidLength) {
  String uidStr = "";
  for (uint8_t i = 0; i < uidLength; i++) {
    if (i > 0) uidStr += " ";
    if (uid[i] < 0x10) uidStr += "0";
    uidStr += String(uid[i], HEX);
  }
  uidStr.toUpperCase();
  
  Serial.print("Found Card UID: ");
  Serial.println(uidStr);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.println("Verifying Card...");
  display.setCursor(10, 35);
  display.print("UID: "); display.println(uidStr);
  display.display();
  
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    
    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    String requestBody = "{\"uid\":\"" + uidStr + "\"}";
    int httpResponseCode = http.POST(requestBody);
    
    if (httpResponseCode > 0) {
      String payload = http.getString();
      Serial.println("Response Payload: " + payload);
      
      display.clearDisplay();
      
      if (payload.indexOf("\"status\":\"success\"") != -1) {
        String username = "Unknown";
        int nameIdx = payload.indexOf("\"username\":\"");
        if (nameIdx != -1) {
          int endIdx = payload.indexOf("\"", nameIdx + 12);
          if (endIdx != -1) {
            username = payload.substring(nameIdx + 12, endIdx);
          }
        }
        
        display.setTextSize(2);
        display.setCursor(10, 10);
        display.println("APPROVED");
        display.setTextSize(1);
        display.setCursor(10, 35);
        display.print("User: "); display.println(username);
        display.setCursor(10, 48);
        display.println("Access Granted!");
        display.display();
        
        // 부저 삐빅 출력
        tone(BUZZER_PIN, 2000, 100);
        delay(150);
        tone(BUZZER_PIN, 2000, 100);
        
      } else {
        display.setTextSize(2);
        display.setCursor(10, 10);
        display.println("DENIED");
        display.setTextSize(1);
        display.setCursor(10, 35);
        display.println("Unregistered Card");
        display.setCursor(10, 48);
        display.println("Access Blocked!");
        display.display();
        
        // 부저 삐~~~ 출력 (800ms)
        tone(BUZZER_PIN, 800, 800);
      }
      
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
      
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(10, 20);
      display.println("Server Conn Error");
      display.setCursor(10, 35);
      display.printf("HTTP Code: %d", httpResponseCode);
      display.display();
      
      tone(BUZZER_PIN, 500, 600);
    }
    
    http.end();
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(10, 20);
    display.println("WiFi Disconnected");
    display.display();
    
    tone(BUZZER_PIN, 500, 600);
  }
  
  delay(2000);
}
