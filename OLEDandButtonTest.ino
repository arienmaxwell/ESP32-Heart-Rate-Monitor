#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define userButton 32
#define DISP_WIDTH 128
#define DISP_HEIGHT 64

#define DISP_RESET -1
Adafruit_SSD1306 oled(DISP_WIDTH, DISP_HEIGHT, &Wire, DISP_RESET);

byte btnState;

void setup() {
  Serial.begin(115200);
  Serial.println();
  delay(2000);

  pinMode(userButton, INPUT_PULLUP);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Display init failed"));
    for(;;);
  }
}

void loop() {
  btnState = digitalRead(userButton);

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.setCursor(0, 0);
  oled.print("Button : ");
  oled.print(btnState);
  oled.display(); 
  delay(250);
}
