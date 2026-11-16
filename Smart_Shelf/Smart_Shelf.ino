/*
  Smart Shelf System - ESP8266
  IR HIGH = object absent -> motor ON
  IR LOW  = object present -> motor OFF
  LCD: 16x2 I2C, address 0x27
  Cloud logging: Google Apps Script -> Google Sheets
*/

#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

/* Enter your own credentials. Do not publish real secrets. */
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* GOOGLE_SCRIPT_URL =
  "https://script.google.com/macros/s/YOUR_DEPLOYMENT_ID/exec";

/* Pin configuration */
constexpr uint8_t IR_PIN = D5;
constexpr uint8_t MOTOR_IN1 = D8;
constexpr uint8_t MOTOR_IN2 = D0;
constexpr uint8_t LCD_SDA_PIN = D2;
constexpr uint8_t LCD_SCL_PIN = D1;

/* Confirm this polarity against your actual IR module. */
constexpr uint8_t OBJECT_ABSENT_LEVEL = HIGH;

LiquidCrystal_PCF8574 lcd(0x27);

bool currentObjectAbsent = false;
bool stateInitialized = false;
unsigned long lastWiFiAttempt = 0;
const unsigned long WIFI_RETRY_INTERVAL_MS = 10000;

void motorOn() {
  digitalWrite(MOTOR_IN1, HIGH);
  digitalWrite(MOTOR_IN2, LOW);
}

void motorOff() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
}

void updateLCD(bool objectAbsent) {
  lcd.setCursor(0, 0);
  lcd.print(objectAbsent ? "IR: ABSENT      " : "IR: PRESENT     ");
  lcd.setCursor(0, 1);
  lcd.print(objectAbsent ? "Motor: ON       " : "Motor: OFF      ");
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print(F("Connecting to Wi-Fi"));
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000UL) {
    delay(250);
    Serial.print('.');
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\nWi-Fi connected."));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("\nWi-Fi timeout. Local shelf control remains active."));
  }
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  if (millis() - lastWiFiAttempt >= WIFI_RETRY_INTERVAL_MS) {
    lastWiFiAttempt = millis();
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println(F("Reconnecting to Wi-Fi..."));
  }
}

/* Sends one record when the shelf state changes. */
void sendToGoogleSheets(bool objectAbsent) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("Cloud log skipped: Wi-Fi disconnected."));
    return;
  }

  BearSSL::WiFiClientSecure client;
  client.setInsecure(); // Prototype convenience; certificate validation is preferable in production.

  HTTPClient https;
  https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  https.setTimeout(2500);

  String irText = objectAbsent ? "ABSENT" : "PRESENT";
  String motorText = objectAbsent ? "ON" : "OFF";
  String url = String(GOOGLE_SCRIPT_URL) +
               "?ir=" + irText +
               "&motor=" + motorText +
               "&weight=0";

  if (!https.begin(client, url)) {
    Serial.println(F("Could not start HTTPS request."));
    return;
  }

  int httpCode = https.GET();
  Serial.print(F("Google Sheets HTTP code: "));
  Serial.println(httpCode);
  if (httpCode > 0) Serial.println(https.getString());
  else Serial.println(https.errorToString(httpCode));
  https.end();
}

void applyState(bool objectAbsent) {
  currentObjectAbsent = objectAbsent;

  if (objectAbsent) motorOn();
  else motorOff();

  updateLCD(objectAbsent);

  Serial.print(F("IR: "));
  Serial.print(objectAbsent ? F("ABSENT") : F("PRESENT"));
  Serial.print(F(" | Motor: "));
  Serial.println(objectAbsent ? F("ON") : F("OFF"));

  sendToGoogleSheets(objectAbsent);
}

void setup() {
  Serial.begin(9600);
  delay(100);

  pinMode(IR_PIN, INPUT);
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  motorOff(); // Safe state during startup.

  Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
  lcd.begin(16, 2);
  lcd.setBacklight(255);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Shelf");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  connectWiFi();

  bool initialAbsent = (digitalRead(IR_PIN) == OBJECT_ABSENT_LEVEL);
  stateInitialized = true;
  applyState(initialAbsent);
}

void loop() {
  // Read and apply local sensor state on every pass through loop().
  bool objectAbsent = (digitalRead(IR_PIN) == OBJECT_ABSENT_LEVEL);

  // Update motor/LCD and log only when the state changes.
  if (!stateInitialized || objectAbsent != currentObjectAbsent) {
    stateInitialized = true;
    applyState(objectAbsent);
  }

  maintainWiFi();
  yield();
}
