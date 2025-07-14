/*
  Basic ESP32-C3 Test Program
  Tests serial communication and basic functionality
*/

#define LED_PIN 2  // Try pin 2 first, common for ESP32-C3

void setup() {
  // Initialize serial at 9600 baud
  Serial.begin(9600);
  delay(3000);  // Give more time for serial to initialize
  
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  
  // Send test messages
  Serial.println();
  Serial.println("=== ESP32-C3 Basic Test ===");
  Serial.println("Serial communication working!");
  Serial.print("Testing LED on pin: ");
  Serial.println(LED_PIN);
  Serial.println("========================");
}

void loop() {
  Serial.println("LED ON");
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  
  Serial.println("LED OFF");  
  digitalWrite(LED_PIN, LOW);
  delay(1000);
}
