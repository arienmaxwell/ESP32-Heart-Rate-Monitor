#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define HeartSensor_PIN 36
#define statusLED       23
#define modeButton      32

#define DISP_WIDTH 128
#define DISP_HEIGHT 64
#define DISP_RESET -1

Adafruit_SSD1306 oled(DISP_WIDTH, DISP_HEIGHT, &Wire, DISP_RESET);

unsigned long timePrevPulse = 0;
unsigned long timePrevBPM = 0;

const long readInterval = 20;
const long bpmInterval = 1000;

int readTimer = 0;
int pulseRaw = 0;
int pulseMax = 520;
int pulseMin = 500;

int beatCounter = 0;
bool pulseFlag = true;
int currentBPM = 0;

int px = 0, py = 0;
int pxLast = 0, pyLast = 0;

bool bpmActive = false;

const unsigned char HeartIcon [] PROGMEM = {
  0x00, 0x00, 0x18, 0x30, 0x3c, 0x78, 0x7e, 0xfc,
  0xff, 0xfe, 0xff, 0xfe, 0xee, 0xee, 0xd5, 0x56,
  0x7b, 0xbc, 0x3f, 0xf8, 0x1f, 0xf0, 0x0f, 0xe0,
  0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00
};

void calculateHeartRate() {
  unsigned long currentMillis = millis();

  if (currentMillis - timePrevPulse >= readInterval) {
    timePrevPulse = currentMillis;

    pulseRaw = analogRead(HeartSensor_PIN);

    if (pulseRaw > pulseMax && pulseFlag) {
      if (bpmActive) beatCounter++;
      pulseFlag = false;
      digitalWrite(statusLED, HIGH);
    }

    if (pulseRaw < pulseMin) {
      pulseFlag = true;
      digitalWrite(statusLED, LOW);
    }

    drawPulseGraph();
  }

  if (millis() - timePrevBPM >= bpmInterval) {
    timePrevBPM = millis();

    if (bpmActive) {
      readTimer++;
      if (readTimer > 10) {
        readTimer = 1;
        currentBPM = beatCounter * 6;
        Serial.print("BPM : ");
        Serial.println(currentBPM);

        oled.fillRect(20, 48, 108, 18, BLACK);
        oled.drawBitmap(0, 47, HeartIcon, 16, 16, WHITE);
        oled.drawLine(0, 43, 127, 43, WHITE);

        oled.setTextSize(2);
        oled.setTextColor(WHITE);
        oled.setCursor(20, 48);
        oled.print(": ");
        oled.print(currentBPM);
        oled.setCursor(92, 48);
        oled.print("BPM");
        oled.display();

        beatCounter = 0;
      }
    }
  }
}

void drawPulseGraph() {
  if (px > 127) {
    oled.fillRect(0, 0, 128, 42, BLACK);
    px = 0;
    pxLast = 0;
  }

  int val = pulseRaw;
  val = constrain(val, 350, 850);
  int mappedY = map(val, 350, 850, 0, 40);
  py = 40 - mappedY;

  oled.writeLine(pxLast, pyLast, px, py, WHITE);
  oled.display();

  pxLast = px;
  pyLast = py;
  px++;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(2000);

  analogReadResolution(10);
  pinMode(statusLED, OUTPUT);
  pinMode(modeButton, INPUT_PULLUP);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Display init failed"));
    for(;;);
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

  oled.clearDisplay(); 
  oled.drawLine(0, 43, 127, 43, WHITE);
  oled.setTextSize(2);
  oled.setCursor(10, 48);
  oled.print("HeartBeat");
  oled.display();
}

void loop() {
  if (digitalRead(modeButton) == LOW) {
    delay(100);

    beatCounter = 0;
    currentBPM = 0;
    px = 0;
    py = 0;
    pxLast = 0;
    pyLast = 0;

    bpmActive = !bpmActive;

    if (bpmActive) {
      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setCursor(14, 0);
      oled.print("Start Getting BPM");
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

      delay(500);

      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setCursor(0, 12);
      oled.print("     Please wait");
      oled.setCursor(0, 22);
      oled.print("     10  seconds");
      oled.setCursor(0, 32);
      oled.print("       to get");
      oled.setCursor(0, 42);
      oled.print("    the BPM value");
      oled.display();
      delay(3000);

      oled.clearDisplay();
      oled.drawBitmap(0, 47, HeartIcon, 16, 16, WHITE);
      oled.drawLine(0, 43, 127, 43, WHITE);
      oled.setTextSize(2);
      oled.setCursor(20, 48);
      oled.print(": 0 ");
      oled.setCursor(92, 48);
      oled.print("BPM");
      oled.display();
    } else {
      oled.clearDisplay();
      oled.setTextSize(2);
      oled.setCursor(42, 25);
      oled.print("STOP");
      oled.display();
      delay(2000);

      oled.clearDisplay();
      oled.drawLine(0, 43, 127, 43, WHITE);
      oled.setTextSize(2);
      oled.setCursor(10, 48);
      oled.print("HeartBeat");
      oled.display();
    }
  }

  calculateHeartRate();
}
