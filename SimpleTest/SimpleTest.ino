/*
  Basic ESP32-C3 Test Program
  Tests serial communication and basic functionality
*/

#define LED_PIN -1  // No built-in LED on ESP32-C3 module

void setup() {
  // Initialize serial at 115200 baud (ESP32-C3 default)
  Serial.begin(115200);
  delay(5000);  // Give more time for serial to initialize
  
  // Initialize LED pin (only if available)
  if (LED_PIN >= 0) {
    pinMode(LED_PIN, OUTPUT);
  }
  
  // Send test messages with more aggressive output
  for(int i = 0; i < 10; i++) {
    Serial.println();
    Serial.println("=== ESP32-C3 STARTUP TEST ===");
    Serial.println("Serial communication working!");
    if (LED_PIN >= 0) {
      Serial.print("Testing LED on pin: ");
      Serial.println(LED_PIN);
    } else {
      Serial.println("No built-in LED on this module");
    }
    Serial.print("Chip Model: ");
    Serial.println(ESP.getChipModel());
    Serial.print("Chip Revision: ");
    Serial.println(ESP.getChipRevision());
    Serial.println("==========================");
    delay(500);
  }
}

void loop() {
  if (LED_PIN >= 0) {
    Serial.println(">>> LED ON - ESP32-C3 ALIVE! <<<");
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    
    Serial.println(">>> LED OFF - ESP32-C3 ALIVE! <<<");  
    digitalWrite(LED_PIN, LOW);
    delay(500);
  } else {
    Serial.println(">>> ESP32-C3 ALIVE! (No LED) <<<");
    delay(1000);
  }
}
