#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>

#include "PageIndex.h"  // Your web interface HTML

#define pulseInputPin   36
#define heartbeatLED    23
#define toggleSwitch    32

#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_RESET      -1

Adafruit_SSD1306 screen(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

unsigned long lastPulseRead = 0;
unsigned long lastBPMCalc = 0;

const long pulseInterval = 35;
const long bpmInterval = 1000;

int pulseCountTimer = 0;
int rawPulseValue = 0;
int upperLimit = 520;
int lowerLimit = 500;

int beatTally = 0;
bool pulseEdge = true;
int bpmResult = 0;

int plotX = 0, plotY = 0;
int prevPlotX = 0, prevPlotY = 0;

bool isBPMActive = false;

byte seconds = 0, minutes = 0, hours = 0;
char timeFormatted[10];

const char* COMMAND_PARAM = "BTN_Start_Get_BPM";
String bpmToggleCommand = "";

JSONVar sensorJSON;

AsyncWebServer apServer(80);
AsyncEventSource apEvents("/events");

const char* apSSID = "ESP32_WS";
const char* apPassword = "helloesp32WS";

IPAddress apIP(192,168,1,1);
IPAddress apGateway(192,168,1,1);
IPAddress apSubnet(255,255,255,0);

const unsigned char HeartShape [] PROGMEM = {
  0x00, 0x00, 0x18, 0x30, 0x3c, 0x78, 0x7e, 0xfc,
  0xff, 0xfe, 0xff, 0xfe, 0xee, 0xee, 0xd5, 0x56,
  0x7b, 0xbc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0xe0,
  0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00
};

void drawPulseGraph() {
  if (plotX > 127) {
    screen.fillRect(0, 0, 128, 42, BLACK);
    plotX = 0;
    prevPlotX = 0;
  }

  int cleanSignal = rawPulseValue;
  cleanSignal = constrain(cleanSignal, 350, 850);
  int mappedY = map(cleanSignal, 350, 850, 0, 40);
  plotY = 40 - mappedY;

  screen.writeLine(prevPlotX, prevPlotY, plotX, plotY, WHITE);
  screen.display();

  prevPlotX = plotX;
  prevPlotY = plotY;
  plotX++;
}

void calculateBPM() {
  if (millis() - lastPulseRead >= pulseInterval) {
    lastPulseRead = millis();

    rawPulseValue = analogRead(pulseInputPin);

    if (rawPulseValue > upperLimit && pulseEdge) {
      if (isBPMActive) beatTally++;
      pulseEdge = false;
      digitalWrite(heartbeatLED, HIGH);
    }

    if (rawPulseValue < lowerLimit) {
      pulseEdge = true;
      digitalWrite(heartbeatLED, LOW);
    }

    drawPulseGraph();

    sensorJSON["signalValue"] = rawPulseValue;
    sensorJSON["timestamp"] = timeFormatted;
    sensorJSON["bpm"] = bpmResult;
    sensorJSON["monitoring"] = isBPMActive;

    String jsonPayload = JSON.stringify(sensorJSON);
    apEvents.send(jsonPayload.c_str(), "bpmUpdate", millis());
  }

  if (millis() - lastBPMCalc >= bpmInterval) {
    lastBPMCalc = millis();

    if (isBPMActive) {
      pulseCountTimer++;

      if (pulseCountTimer > 10) {
        pulseCountTimer = 1;
        seconds += 10;
        if (seconds >= 60) { seconds = 0; minutes++; }
        if (minutes >= 60) { minutes = 0; hours++; }

        sprintf(timeFormatted, "%02d:%02d:%02d", hours, minutes, seconds);
        bpmResult = beatTally * 6;
        beatTally = 0;

        Serial.print("BPM: ");
        Serial.println(bpmResult);

        screen.fillRect(20, 48, 108, 18, BLACK);
        screen.drawBitmap(0, 47, HeartShape, 16, 16, WHITE);
        screen.drawLine(0, 43, 127, 43, WHITE);
        screen.setTextSize(2);
        screen.setCursor(20, 48);
        screen.print(": ");
        screen.print(bpmResult);
        screen.setCursor(92, 48);
        screen.print("BPM");
        screen.display();
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(10);

  pinMode(heartbeatLED, OUTPUT);
  pinMode(toggleSwitch, INPUT_PULLUP);
  sprintf(timeFormatted, "%02d:%02d:%02d", hours, minutes, seconds);

  if (!screen.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED Failed"));
    for (;;);
  }

  screen.clearDisplay();
  screen.setTextColor(WHITE);
  screen.setTextSize(2);
  screen.setCursor(30, 0); 
  screen.print("HEART");
  screen.setCursor(20, 20); 
  screen.print("MONITOR");
  screen.setCursor(25, 40); 
  screen.print("AP MODE");
  screen.display();
  delay(2000);

  Serial.println("Setting up Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  WiFi.softAPConfig(apIP, apGateway, apSubnet);

  Serial.println("AP Ready");
  Serial.print("SSID: ");
  Serial.println(apSSID);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  screen.clearDisplay();
  screen.setTextSize(1);
  screen.setCursor(0, 0);
  screen.print("ESP32 IP address:");
  screen.setCursor(0, 10);
  screen.print(WiFi.softAPIP());
  screen.display();
  delay(3000);

  // Web routes
  apServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", MAIN_page);
  });

  apServer.on("/BTN_Comd", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam(COMMAND_PARAM)) {
      bpmToggleCommand = request->getParam(COMMAND_PARAM)->value();
      Serial.print("Toggle Command: ");
      Serial.println(bpmToggleCommand);
    }
    request->send(200, "text/plain", "OK");
  });

  apEvents.onConnect([](AsyncEventSourceClient *client) {
    client->send("connected!", NULL, millis(), 10000);
  });

  apServer.addHandler(&apEvents);
  apServer.begin();

  screen.clearDisplay();
  screen.drawLine(0, 43, 127, 43, WHITE);
  screen.setTextSize(2);
  screen.setCursor(10, 48);
  screen.print("HeartBeat");
  screen.display();
}

void loop() {
  if (digitalRead(toggleSwitch) == LOW || bpmToggleCommand == "START" || bpmToggleCommand == "STOP") {
    delay(100);
    bpmToggleCommand = "";

    beatTally = 0;
    bpmResult = 0;
    plotX = prevPlotX = plotY = prevPlotY = 0;
    seconds = minutes = hours = 0;
    sprintf(timeFormatted, "%02d:%02d:%02d", hours, minutes, seconds);

    isBPMActive = !isBPMActive;

    screen.clearDisplay();
    screen.setTextSize(2);
    screen.setTextColor(WHITE);

    if (isBPMActive) {
      screen.setCursor(25, 20);
      screen.setTextSize(1);
      screen.print("Monitoring...");
      screen.display();

      screen.setTextSize(3);
      for (int i = 3; i > 0; i--) {
        screen.setCursor(50, 30);
        screen.print(i);
        screen.display();
        delay(1000);
        screen.setTextColor(BLACK);
        screen.setCursor(50, 30);
        screen.print(i);
        screen.display();
        screen.setTextColor(WHITE);
      }
    } else {
      screen.setCursor(42, 25);
      screen.print("STOP");
      screen.display();
      delay(1000);
    }

    screen.clearDisplay();
    screen.drawLine(0, 43, 127, 43, WHITE);
    screen.setCursor(10, 48);
    screen.print("HeartBeat");
    screen.display();
  }

  calculateBPM();
}
