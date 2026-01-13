#include <WiFi.h>
#include <time.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EEPROM.h>

// ---------- WIFI ----------
const char* ssid     = "BBG_Dougnet";
const char* password = "Custela02";

// ---------- NTP ----------
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -3 * 3600;  // Brasil
const int   daylightOffset_sec = 0;

// ---------- PINOS ----------
#define DS18B20_PIN 33
#define RELAY_PIN   2
#define SDA_PIN 4
#define SCL_PIN 16

// ---------- EEPROM ----------
#define EEPROM_SIZE 64
#define ADDR_TEMP_MAX 0
#define ADDR_TEMP_MIN 4

// ---------- TEMPERATURAS ----------
#define TEMP_LIGA     32.0
#define TEMP_DESLIGA  30.0

// ---------- DS18B20 ----------
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

// ---------- OLED ----------
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(
  U8G2_R0, U8X8_PIN_NONE
);

bool releLigado = false;
float tempMax, tempMin;

// ---------- EEPROM ----------
void salvarEEPROM() {
  EEPROM.put(ADDR_TEMP_MAX, tempMax);
  EEPROM.put(ADDR_TEMP_MIN, tempMin);
  EEPROM.commit();
}

// ---------- NTP ----------
void conectarWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  sensors.begin();

  // EEPROM
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(ADDR_TEMP_MAX, tempMax);
  EEPROM.get(ADDR_TEMP_MIN, tempMin);

  if (isnan(tempMax) || tempMax < -50 || tempMax > 150) tempMax = -100;
  if (isnan(tempMin) || tempMin < -50 || tempMin > 150) tempMin = 100;

  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin();

  // WiFi + NTP
  conectarWiFi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(20, 30, "Sistema Ativo");
  display.sendBuffer();
  delay(500);
}

void loop() {
  sensors.requestTemperatures();
  float temperatura = sensors.getTempCByIndex(0);

  if (temperatura < -55 || temperatura > 125) return;

  bool salvar = false;

  if (temperatura > tempMax) { tempMax = temperatura; salvar = true; }
  if (temperatura < tempMin) { tempMin = temperatura; salvar = true; }

  if (salvar) salvarEEPROM();

  // Relé
  if (temperatura >= TEMP_LIGA && !releLigado) {
    releLigado = true;
    digitalWrite(RELAY_PIN, HIGH);
  }

  if (temperatura <= TEMP_DESLIGA && releLigado) {
    releLigado = false;
    digitalWrite(RELAY_PIN, LOW);
  }


   if (WiFi.status() == WL_CONNECTED) {
    drawWiFiIconBars(108, 63); // canto inferior direito
}
  // Hora/Data
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char dataStr[20];
  char horaStr[10];

  strftime(dataStr, sizeof(dataStr), "%d/%m/%Y", &timeinfo);
  strftime(horaStr, sizeof(horaStr), "%H:%M:%S", &timeinfo);

  // OLED
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tf);

  display.drawStr(0, 10, dataStr);
  display.drawStr(78, 10, horaStr);

  char buf[10];
  dtostrf(temperatura, 4, 1, buf);

  display.setFont(u8g2_font_logisoso24_tf);
  display.drawStr(0, 45, buf);
  display.setFont(u8g2_font_6x12_tf);
  display.drawStr(58, 45, "C");

  display.drawLine(0, 48, 127, 48);

  dtostrf(tempMax, 4, 1, buf);
  display.drawStr(108, 30, "Max");
  display.drawStr(80, 30, buf);

  dtostrf(tempMin, 4, 1, buf);
  display.drawStr(108, 40, "Min");
  display.drawStr(80, 40, buf);

  display.drawStr(0, 63, "Rele:");
  display.drawStr(40, 63, releLigado ? "LIGADO" : "DESLIGADO");

  if (WiFi.status() == WL_CONNECTED) {
   drawWiFiIconBars(108, 63);
}

  display.sendBuffer();

  delay(500);
}

void drawWiFiIconBars(int x, int y) {
  // largura de cada barra
  int w = 3;

  // barras (da menor para a maior)
  display.drawBox(x + 0, y - 3,  w, 3);   // barra 1
  display.drawBox(x + 5, y - 6,  w, 6);   // barra 2
  display.drawBox(x + 10, y - 9, w, 9);   // barra 3
  display.drawBox(x + 15, y - 12, w, 12); // barra 4
}


