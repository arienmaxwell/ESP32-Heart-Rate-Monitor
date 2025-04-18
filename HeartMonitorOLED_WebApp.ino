#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>

#include "PageIndex.h"  // Contains HTML/CSS/JS

#define SensorInputPin 36 
#define ledOutputPin   23 
#define toggleButton   32

#define DISPLAY_W 128
#define DISPLAY_H 64
#define DISPLAY_RESET -1

Adafruit_SSD1306 oled(DISPLAY_W, DISPLAY_H, &Wire, DISPLAY_RESET);

const char* wifiSSID = "REPLACE_WITH_YOUR_SSID";
const char* wifiPASS = "REPLACE_WITH_YOUR_PASSWORD";

unsigned long lastReadMillis = 0;
unsigned long lastBPMMillis = 0;

const long sensorInterval = 35;
const long bpmCalcInterval = 1000;

int tenSecTimer = 0;
int analogPulse = 0;
int maxThreshold = 520;
int minThreshold = 500;

int beatCount = 0;
bool beatState = true;
int finalBPM = 0;

int xPlot = 0, yPlot = 0;
int xOld = 0, yOld = 0;

bool isMonitoring = false;

byte secCounter = 0, minCounter = 0, hourCounter = 0;
char timeBuffer[10];

const char* PARAM_CMD = "BTN_Start_Get_BPM";
String btnCommand = "";

JSONVar jsonOutput;

AsyncWebServer web(80);
AsyncEventSource stream("/events");

const unsigned char IconHeart [] PROGMEM = {
  0x00, 0x00, 0x18, 0x30, 0x3c, 0x78, 0x7e, 0xfc, 0xff, 0xfe, 0xff, 0xfe, 0xee, 0xee, 0xd5, 0x56, 
  0x7b, 0xbc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00
};

void updateGraph() {
  if (xPlot > 127) {
    oled.fillRect(0, 0, 128, 42, BLACK);
    xPlot = 0;
    xOld = 0;
  }

  int cleanVal = analogPulse;
  cleanVal = constrain(cleanVal, 350, 850);
  int mappedY = map(cleanVal, 350, 850, 0, 40);
  yPlot = 40 - mappedY;

  oled.writeLine(xOld, yOld, xPlot, yPlot, WHITE);
  oled.display();

  xOld = xPlot;
  yOld = yPlot;
  xPlot++;
}

void measureHeartRate() {
  if (millis() - lastReadMillis >= sensorInterval) {
    lastReadMillis = millis();

    analogPulse = analogRead(SensorInputPin);

    if (analogPulse > maxThreshold && beatState) {
      if (isMonitoring) beatCount++;
      beatState = false;
      digitalWrite(ledOutputPin, HIGH);
    }

    if (analogPulse < minThreshold) {
      beatState = true;
      digitalWrite(ledOutputPin, LOW);
    }

    updateGraph();

    jsonOutput["pulseGraph"] = analogPulse;
    jsonOutput["timestamp"] = timeBuffer;
    jsonOutput["rateBPM"] = finalBPM;
    jsonOutput["tracking"] = isMonitoring;

    String jsonText = JSON.stringify(jsonOutput);
    stream.send(jsonText.c_str(), "bpmData", millis());
  }

  if (millis() - lastBPMMillis >= bpmCalcInterval) {
    lastBPMMillis = millis();

    if (isMonitoring) {
      tenSecTimer++;

      if (tenSecTimer > 10) {
        tenSecTimer = 1;
        secCounter += 10;
        if (secCounter >= 60) { secCounter = 0; minCounter++; }
        if (minCounter >= 60) { minCounter = 0; hourCounter++; }

        sprintf(timeBuffer, "%02d:%02d:%02d", hourCounter, minCounter, secCounter);

        finalBPM = beatCount * 6;
        Serial.print("BPM: ");
        Serial.println(finalBPM);

        oled.fillRect(20, 48, 108, 18, BLACK);
        oled.drawBitmap(0, 47, IconHeart, 16, 16, WHITE);
        oled.drawLine(0, 43, 127, 43, WHITE);
        oled.setTextSize(2);
        oled.setTextColor(WHITE);
        oled.setCursor(20, 48);
        oled.print(": ");
        oled.print(finalBPM);
        oled.setCursor(92, 48);
        oled.print("BPM");
        oled.display();

        beatCount = 0;
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(10);

  pinMode(ledOutputPin, OUTPUT);
  pinMode(toggleButton, INPUT_PULLUP);
  sprintf(timeBuffer, "%02d:%02d:%02d", hourCounter, minCounter, secCounter);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Display fail"));
    for (;;);
  }

  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(2);
  oled.setCursor(37, 0); 
  oled.print("ESP32"); 
  oled.setCursor(13, 20); 
  oled.print("HEARTBEAT"); 
  oled.setCursor(7, 40); 
  oled.print("MONITORING"); 
  oled.display();
  delay(2000);

  Serial.println("WiFi STA Mode");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID, wifiPASS);

  int timeout = 20 * 2;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (--timeout == 0) {
      ESP.restart();
    }
  }

  Serial.println("WiFi connected.");
  oled.clearDisplay();
  oled.setCursor(0, 0); 
  oled.print("Connected OK");
  oled.display();
  delay(1000);

  web.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", MAIN_page);
  });

  web.on("/BTN_Comd", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam(PARAM_CMD)) {
      btnCommand = request->getParam(PARAM_CMD)->value();
      Serial.print("Command: ");
      Serial.println(btnCommand);
    } else {
      btnCommand = "No input";
    }
    request->send(200, "text/plain", "OK");
  });

  stream.onConnect([](AsyncEventSourceClient *client) {
    client->send("hello!", NULL, millis(), 10000);
  });

  web.addHandler(&stream);
  web.begin();

  oled.clearDisplay();
  oled.setCursor(0, 0); 
  oled.print("Server Started");
  oled.setCursor(0, 10); 
  oled.print(WiFi.localIP());
  oled.display();
  delay(3000);

  oled.clearDisplay();
  oled.drawLine(0, 43, 127, 43, WHITE);
  oled.setCursor(10, 48);
  oled.print("HeartBeat");
  oled.display();
}

void loop() {
  if (digitalRead(toggleButton) == LOW || btnCommand == "START" || btnCommand == "STOP") {
    delay(100);
    btnCommand = "";
    beatCount = 0;
    finalBPM = 0;
    xPlot = yPlot = xOld = yOld = 0;
    secCounter = minCounter = hourCounter = 0;
    sprintf(timeBuffer, "%02d:%02d:%02d", hourCounter, minCounter, secCounter);
    isMonitoring = !isMonitoring;

    oled.clearDisplay();
    oled.setTextColor(WHITE);
    oled.setTextSize(2);

    if (isMonitoring) {
      oled.setCursor(14, 0);
      oled.setTextSize(1);
      oled.print("Monitoring...");
      oled.setTextSize(3);
      for (byte i = 3; i > 0; i--) {
        oled.setCursor(50, 20);
        oled.print(i);
        oled.display();
        delay(1000);
        oled.setTextColor(BLACK);
        oled.setCursor(50, 20);
        oled.print(i);
        oled.display();
        oled.setTextColor(WHITE);
      }
    } else {
      oled.setCursor(42, 25);
      oled.print("STOP");
      oled.display();
      delay(2000);
    }

    oled.clearDisplay();
    oled.drawLine(0, 43, 127, 43, WHITE);
    oled.setCursor(10, 48);
    oled.print("HeartBeat");
    oled.display();
  }

  measureHeartRate();
}
