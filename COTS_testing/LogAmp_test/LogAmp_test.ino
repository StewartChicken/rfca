
// Teensy uses a 3.3V reference for analog inputs
const float ADC_REF_VOLTAGE = 3.3;  
const int   ADC_MAX_VALUE   = 1023; // default 10-bit resolution

void setup() {
  Serial.begin(9600);
  delay(500);

  analogReadResolution(10);   // 10 bit resolution

  Serial.println("Reading A16...");
}

void loop() {
  int raw = analogRead(A16);

  // Convert raw ADC value to voltage
  float voltage = (raw * ADC_REF_VOLTAGE) / ADC_MAX_VALUE;
  Serial.println(voltage);

  delay(200);
}
