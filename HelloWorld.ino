/*
  Hello World Program for ESP32-C3 with GC9A01 Round LCD
  Working version using direct SPI control to bypass TFT_eSPI bugs
  Based on research from GitHub issue #3772
*/

#include <SPI.h>

// Pin definitions for ESP32-C3 with round GC9A01 display - CORRECTED PINS
#define TFT_SCLK 6   // SPI Clock
#define TFT_MOSI 7   // SPI Data Output (MOSI)
#define TFT_CS   10  // Chip Select
#define TFT_DC   2   // Data/Command select
#define TFT_RST  -1  // Reset pin (not connected)
#define TFT_BL   3   // Backlight control
#define LED_BUILTIN -1  // ESP32-C3 module has no built-in LED

// Screen resolution
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// SPI instance for ESP32-C3
SPIClass mySPI(FSPI); // Use FSPI (ESP32-C3's SPI2)

void tft_send_cmd(uint8_t cmd) {
  digitalWrite(TFT_DC, LOW);  // Command mode
  digitalWrite(TFT_CS, LOW);
  mySPI.transfer(cmd);
  digitalWrite(TFT_CS, HIGH);
}

void tft_send_data(uint8_t data) {
  digitalWrite(TFT_DC, HIGH); // Data mode
  digitalWrite(TFT_CS, LOW);
  mySPI.transfer(data);
  digitalWrite(TFT_CS, HIGH);
}

void tft_init() {
  // Initialize GPIO pins
  Serial.println("Setting up GPIO pins...");
  pinMode(TFT_DC, OUTPUT);
  if (TFT_RST >= 0) pinMode(TFT_RST, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_BL, OUTPUT);
  Serial.println("GPIO pins configured");
  
  // Turn on backlight
  digitalWrite(TFT_BL, HIGH);
  Serial.println("Backlight ON - Pin 3 set HIGH");
  
  // Test all pins by toggling them
  Serial.println("Testing pins...");
  if (TFT_RST >= 0) digitalWrite(TFT_RST, HIGH);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TFT_DC, LOW);
  Serial.println("Pins initialized");
  
  // Reset sequence - only if reset pin is connected
  if (TFT_RST >= 0) {
    Serial.println("Starting display reset...");
    digitalWrite(TFT_RST, LOW);
    delay(100);
    digitalWrite(TFT_RST, HIGH);
    delay(120);
    Serial.println("Display reset complete");
  } else {
    Serial.println("Reset pin not connected - skipping hardware reset");
    delay(200); // Give display time to stabilize
  }

  // Initialize SPI - MISO set to -1 as it's not used
  Serial.println("Initializing SPI...");
  mySPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  mySPI.setFrequency(27000000); // 27MHz
  Serial.println("SPI initialized at 27MHz");

  // Test SPI by sending a simple command
  Serial.println("Testing SPI communication...");
  digitalWrite(TFT_CS, LOW);
  mySPI.transfer(0x00); // NOP command
  digitalWrite(TFT_CS, HIGH);
  Serial.println("SPI test complete");

  // GC9A01 initialization sequence
  Serial.println("Initializing GC9A01 display...");
  
  tft_send_cmd(0xEF);
  tft_send_cmd(0xEB);
  tft_send_data(0x14);
  
  tft_send_cmd(0xFE);
  tft_send_cmd(0xEF);
  
  tft_send_cmd(0xEB);
  tft_send_data(0x14);
  
  tft_send_cmd(0x84);
  tft_send_data(0x40);
  
  tft_send_cmd(0x85);
  tft_send_data(0xFF);
  
  tft_send_cmd(0x86);
  tft_send_data(0xFF);
  
  tft_send_cmd(0x87);
  tft_send_data(0xFF);
  
  tft_send_cmd(0x88);
  tft_send_data(0x0A);
  
  tft_send_cmd(0x89);
  tft_send_data(0x21);
  
  tft_send_cmd(0x8A);
  tft_send_data(0x00);
  
  tft_send_cmd(0x8B);
  tft_send_data(0x80);
  
  tft_send_cmd(0x8C);
  tft_send_data(0x01);
  
  tft_send_cmd(0x8D);
  tft_send_data(0x01);
  
  tft_send_cmd(0x8E);
  tft_send_data(0xFF);
  
  tft_send_cmd(0x8F);
  tft_send_data(0xFF);
  
  tft_send_cmd(0xB6);
  tft_send_data(0x00);
  tft_send_data(0x20);
  
  tft_send_cmd(0x36);
  tft_send_data(0x08);
  
  tft_send_cmd(0x3A);
  tft_send_data(0x05);
  
  tft_send_cmd(0x90);
  tft_send_data(0x08);
  tft_send_data(0x08);
  tft_send_data(0x08);
  tft_send_data(0x08);
  
  tft_send_cmd(0xBD);
  tft_send_data(0x06);
  
  tft_send_cmd(0xBC);
  tft_send_data(0x00);
  
  tft_send_cmd(0xFF);
  tft_send_data(0x60);
  tft_send_data(0x01);
  tft_send_data(0x04);
  
  tft_send_cmd(0xC3);
  tft_send_data(0x13);
  
  tft_send_cmd(0xC4);
  tft_send_data(0x13);
  
  tft_send_cmd(0xC9);
  tft_send_data(0x22);
  
  tft_send_cmd(0xBE);
  tft_send_data(0x11);
  
  tft_send_cmd(0xE1);
  tft_send_data(0x10);
  tft_send_data(0x0E);
  
  tft_send_cmd(0xDF);
  tft_send_data(0x21);
  tft_send_data(0x0C);
  tft_send_data(0x02);
  
  tft_send_cmd(0xF0);
  tft_send_data(0x45);
  tft_send_data(0x09);
  tft_send_data(0x08);
  tft_send_data(0x08);
  tft_send_data(0x26);
  tft_send_data(0x2A);
  
  tft_send_cmd(0xF1);
  tft_send_data(0x43);
  tft_send_data(0x70);
  tft_send_data(0x72);
  tft_send_data(0x36);
  tft_send_data(0x37);
  tft_send_data(0x6F);
  
  tft_send_cmd(0xF2);
  tft_send_data(0x45);
  tft_send_data(0x09);
  tft_send_data(0x08);
  tft_send_data(0x08);
  tft_send_data(0x26);
  tft_send_data(0x2A);
  
  tft_send_cmd(0xF3);
  tft_send_data(0x43);
  tft_send_data(0x70);
  tft_send_data(0x72);
  tft_send_data(0x36);
  tft_send_data(0x37);
  tft_send_data(0x6F);
  
  tft_send_cmd(0xED);
  tft_send_data(0x1B);
  tft_send_data(0x0B);
  
  tft_send_cmd(0xAE);
  tft_send_data(0x77);
  
  tft_send_cmd(0xCD);
  tft_send_data(0x63);
  
  tft_send_cmd(0x70);
  tft_send_data(0x07);
  tft_send_data(0x07);
  tft_send_data(0x04);
  tft_send_data(0x0E);
  tft_send_data(0x0F);
  tft_send_data(0x09);
  tft_send_data(0x07);
  tft_send_data(0x08);
  tft_send_data(0x03);
  
  tft_send_cmd(0xE8);
  tft_send_data(0x34);
  
  tft_send_cmd(0x62);
  tft_send_data(0x18);
  tft_send_data(0x0D);
  tft_send_data(0x71);
  tft_send_data(0xED);
  tft_send_data(0x70);
  tft_send_data(0x70);
  tft_send_data(0x18);
  tft_send_data(0x0F);
  tft_send_data(0x71);
  tft_send_data(0xEF);
  tft_send_data(0x70);
  tft_send_data(0x70);
  
  tft_send_cmd(0x63);
  tft_send_data(0x18);
  tft_send_data(0x11);
  tft_send_data(0x71);
  tft_send_data(0xF1);
  tft_send_data(0x70);
  tft_send_data(0x70);
  tft_send_data(0x18);
  tft_send_data(0x13);
  tft_send_data(0x71);
  tft_send_data(0xF3);
  tft_send_data(0x70);
  tft_send_data(0x70);
  
  tft_send_cmd(0x64);
  tft_send_data(0x28);
  tft_send_data(0x29);
  tft_send_data(0xF1);
  tft_send_data(0x01);
  tft_send_data(0xF1);
  tft_send_data(0x00);
  tft_send_data(0x07);
  
  tft_send_cmd(0x66);
  tft_send_data(0x3C);
  tft_send_data(0x00);
  tft_send_data(0xCD);
  tft_send_data(0x67);
  tft_send_data(0x45);
  tft_send_data(0x45);
  tft_send_data(0x10);
  tft_send_data(0x00);
  tft_send_data(0x00);
  tft_send_data(0x00);
  
  tft_send_cmd(0x67);
  tft_send_data(0x00);
  tft_send_data(0x3C);
  tft_send_data(0x00);
  tft_send_data(0x00);
  tft_send_data(0x00);
  tft_send_data(0x01);
  tft_send_data(0x54);
  tft_send_data(0x10);
  tft_send_data(0x32);
  tft_send_data(0x98);
  
  tft_send_cmd(0x74);
  tft_send_data(0x10);
  tft_send_data(0x85);
  tft_send_data(0x80);
  tft_send_data(0x00);
  tft_send_data(0x00);
  tft_send_data(0x4E);
  tft_send_data(0x00);
  
  tft_send_cmd(0x98);
  tft_send_data(0x3E);
  tft_send_data(0x07);
  
  tft_send_cmd(0x35);
  tft_send_cmd(0x21);
  
  tft_send_cmd(0x11);  // Exit Sleep
  delay(120);
  
  tft_send_cmd(0x29);  // Display on
  delay(20);
  
  Serial.println("GC9A01 initialization complete!");
}

void tft_fill_screen(uint16_t color) {
  // Set column address (full width)
  tft_send_cmd(0x2A);
  tft_send_data(0x00);
  tft_send_data(0x00);
  tft_send_data((TFT_WIDTH >> 8) & 0xFF);
  tft_send_data(TFT_WIDTH & 0xFF);
  
  // Set row address (full height)
  tft_send_cmd(0x2B);
  tft_send_data(0x00);
  tft_send_data(0x00);
  tft_send_data((TFT_HEIGHT >> 8) & 0xFF);
  tft_send_data(TFT_HEIGHT & 0xFF);
  
  // Memory write command
  tft_send_cmd(0x2C);

  // Send color data
  uint8_t hi = color >> 8;
  uint8_t lo = color & 0xFF;
  
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_CS, LOW);
  
  for(uint32_t i = 0; i < TFT_WIDTH * TFT_HEIGHT; i++) {
    mySPI.transfer(hi);
    mySPI.transfer(lo);
  }
  
  digitalWrite(TFT_CS, HIGH);
}

void draw_hello_world() {
  // Fill screen with blue
  tft_fill_screen(0x001F); // Blue
  delay(1000);
  
  // Fill screen with green
  tft_fill_screen(0x07E0); // Green
  delay(1000);
  
  // Fill screen with red
  tft_fill_screen(0xF800); // Red
  delay(1000);
  
  // Fill screen with white
  tft_fill_screen(0xFFFF); // White
  delay(1000);
  
  // Fill screen with black
  tft_fill_screen(0x0000); // Black
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);  // Use 115200 baud like SimpleTest
  
  // Wait for serial port to connect
  delay(3000);
  
  // Print startup message with extra debug info
  Serial.println("=========================================");
  Serial.println("Hello World from ESP32-C3!");
  Serial.println("ESP32-C3 with GC9A01 Round LCD Display");
  Serial.println("Using Direct SPI Control");
  Serial.println("=========================================");
  Serial.print("ESP32 Chip Model: ");
  Serial.println(ESP.getChipModel());
  Serial.print("ESP32 Chip Revision: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("ESP32 CPU Frequency: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");
  Serial.println("Starting initialization...");
  
  // Initialize LED (only if available)
  if (LED_BUILTIN >= 0) {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Built-in LED ON");
  } else {
    Serial.println("No built-in LED on this ESP32-C3 module");
  }
  
  // Initialize display
  Serial.println("Initializing display...");
  tft_init();
  
  // Test display with color fills
  Serial.println("Testing display with colors...");
  draw_hello_world();
  
  Serial.println("Setup complete! Display should show colors.");
}

void loop() {
  // Print status to serial
  Serial.println("Hello World from ESP32-C3!");
  
  // Blink LED (only if available)
  if (LED_BUILTIN >= 0) {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("LED OFF");
    delay(500);
    
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("LED ON");
    delay(500);
  } else {
    // No LED, just delay
    delay(1000);
  }
  
  // Cycle through colors every 10 seconds
  static unsigned long lastColorChange = 0;
  static uint8_t colorIndex = 0;
  
  if (millis() - lastColorChange > 100) {
    uint16_t colors[] = {0x0000, 0x001F, 0x07E0, 0xF800, 0xFFFF}; // Black, Blue, Green, Red, White
    String colorNames[] = {"Black", "Blue", "Green", "Red", "White"};
    
    Serial.println("Changing display color to: " + colorNames[colorIndex]);
    tft_fill_screen(colors[colorIndex]);
    
    colorIndex = (colorIndex + 1) % 5;
    lastColorChange = millis();
  }
}
