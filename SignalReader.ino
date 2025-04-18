#define HeartSensor_PIN 36 
#define statusLED       23 

int pulseValue;

void setup() {
  Serial.begin(115200); 
  Serial.println();
  delay(2000);

  analogReadResolution(10);
  pinMode(statusLED, OUTPUT);
}

void loop() {
  pulseValue = analogRead(HeartSensor_PIN); 
  Serial.println(pulseValue); 
  delay(20);
}

