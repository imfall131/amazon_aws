#include <ESP8266WiFi.h>
#include <time.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

// OLED 핀 정보 (D5 -> GPIO14, D6 -> GPIO12)
#define OLED_SDA 14
#define OLED_SCL 12

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "imfall";
const char* password = "00000000";

// 한국 표준시 (UTC+9)
const int timezone = 9 * 3600;
const int dst = 0;

void setup() {
  Serial.begin(115200);
  
  // I2C 핀 명시적 설정 및 시작
  Wire.begin(OLED_SDA, OLED_SCL);
  
  // SSD1306 디스플레이 초기화
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Connecting to WiFi...");
  display.printf("SSID: %s\n", ssid);
  display.display();
  
  // WiFi 연결
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println("WiFi Connected!");
  display.println("Syncing NTP Time...");
  display.display();
  
  // NTP 동기화 설정 (UTC+9)
  configTime(timezone, dst, "pool.ntp.org", "time.nist.gov");
  
  // 유효한 시간이 올 때까지 대기
  Serial.println("Waiting for NTP time sync...");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nTime synced!");
}

void loop() {
  time_t now = time(nullptr);
  struct tm* timeinfo;
  timeinfo = localtime(&now);
  
  display.clearDisplay();
  
  // 날짜 표시
  display.setTextSize(1);
  display.setCursor(10, 15);
  display.printf("%04d-%02d-%02d", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday);
  
  // 요일 표시
  const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  display.setCursor(95, 15);
  display.print(days[timeinfo->tm_wday]);
  
  // 시간 표시 (HH:MM:SS)
  display.setTextSize(2);
  display.setCursor(15, 35);
  display.printf("%02d:%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
  
  display.display();
  delay(1000);
}
