/*
  Simple Digital Meters Display for ESP32-C3 with GC9A01
  Using direct SPI control (no external libraries needed)
  Features: Multiple digital displays, progress bars, and status indicators
*/

#include <SPI.h>
#include <math.h>

// Pin definitions for ESP32-C3 with round GC9A01 display - CORRECTED PINS
#define TFT_SCLK 6   // SPI Clock
#define TFT_MOSI 7   // SPI Data Output (MOSI)
#define TFT_CS   10  // Chip Select
#define TFT_DC   2   // Data/Command select
#define TFT_RST  -1  // Reset pin (not connected)
#define TFT_BL   3   // Backlight control

// Screen resolution
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// Color definitions (RGB565)
#define BLACK       0x0000
#define WHITE       0xFFFF
#define RED         0xF800
#define GREEN       0x07E0
#define BLUE        0x001F
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define ORANGE      0xFD20
#define GRAY        0x8410
#define DARK_GRAY   0x4208
#define LIGHT_GRAY  0xC618

// SPI instance for ESP32-C3
SPIClass mySPI(FSPI); // Use FSPI (ESP32-C3's SPI2)

// Sensor data structure
struct SensorData {
  float value;
  float target;
  float min, max;
  const char* name;
  const char* unit;
  uint16_t color;
  bool warning;
};

// Define our sensors
SensorData sensors[] = {
  {25.5, 30.0, 0, 60, "TEMP", "C", ORANGE, false},
  {3.28, 3.35, 2.8, 3.6, "VOLT", "V", GREEN, false},
  {78.5, 85.0, 0, 100, "CPU", "%", BLUE, false},
  {45.2, 50.0, 0, 100, "MEM", "%", CYAN, false}
};

const int numSensors = 4;

// Animation variables
unsigned long lastUpdate = 0;
unsigned long lastDataUpdate = 0;
float animationPhase = 0;

// Basic SPI communication functions
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

void tft_set_addr_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  // Set column address
  tft_send_cmd(0x2A);
  tft_send_data(x >> 8);
  tft_send_data(x & 0xFF);
  tft_send_data((x + w - 1) >> 8);
  tft_send_data((x + w - 1) & 0xFF);
  
  // Set row address
  tft_send_cmd(0x2B);
  tft_send_data(y >> 8);
  tft_send_data(y & 0xFF);
  tft_send_data((y + h - 1) >> 8);
  tft_send_data((y + h - 1) & 0xFF);
  
  // Memory write command
  tft_send_cmd(0x2C);
}

void tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  tft_set_addr_window(x, y, w, h);
  
  uint8_t hi = color >> 8;
  uint8_t lo = color & 0xFF;
  
  digitalWrite(TFT_DC, HIGH);
  digitalWrite(TFT_CS, LOW);
  
  for (uint32_t i = 0; i < w * h; i++) {
    mySPI.transfer(hi);
    mySPI.transfer(lo);
  }
  
  digitalWrite(TFT_CS, HIGH);
}

void tft_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
  if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
  tft_fill_rect(x, y, 1, 1, color);
}

void tft_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    // Swap x and y coordinates
    int16_t temp;
    temp = x0; x0 = y0; y0 = temp;
    temp = x1; x1 = y1; y1 = temp;
  }

  if (x0 > x1) {
    // Swap start and end points
    int16_t temp;
    temp = x0; x0 = x1; x1 = temp;
    temp = y0; y0 = y1; y1 = temp;
  }

  int16_t dx = x1 - x0;
  int16_t dy = abs(y1 - y0);
  int16_t err = dx / 2;
  int16_t ystep = (y0 < y1) ? 1 : -1;

  for (; x0 <= x1; x0++) {
    if (steep) {
      tft_draw_pixel(y0, x0, color);
    } else {
      tft_draw_pixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void tft_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  tft_draw_line(x, y, x + w - 1, y, color);         // Top
  tft_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color); // Bottom
  tft_draw_line(x, y, x, y + h - 1, color);         // Left
  tft_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color); // Right
}

void tft_draw_circle(uint16_t x0, uint16_t y0, uint16_t radius, uint16_t color) {
  int x = radius;
  int y = 0;
  int err = 0;

  while (x >= y) {
    tft_draw_pixel(x0 + x, y0 + y, color);
    tft_draw_pixel(x0 + y, y0 + x, color);
    tft_draw_pixel(x0 - y, y0 + x, color);
    tft_draw_pixel(x0 - x, y0 + y, color);
    tft_draw_pixel(x0 - x, y0 - y, color);
    tft_draw_pixel(x0 - y, y0 - x, color);
    tft_draw_pixel(x0 + y, y0 - x, color);
    tft_draw_pixel(x0 + x, y0 - y, color);

    if (err <= 0) {
      y += 1;
      err += 2*y + 1;
    }

    if (err > 0) {
      x -= 1;
      err -= 2*x + 1;
    }
  }
}

// Simple 5x7 font for numbers
const uint8_t font5x7[][5] = {
  {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
  {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
  {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
  {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
  {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
  {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
};

void tft_draw_digit(uint16_t x, uint16_t y, uint8_t digit, uint16_t color, uint8_t size) {
  if (digit > 9) return;
  
  for (int col = 0; col < 5; col++) {
    uint8_t line = font5x7[digit][col];
    for (int row = 0; row < 7; row++) {
      if (line & (1 << row)) {
        tft_fill_rect(x + col * size, y + row * size, size, size, color);
      }
    }
  }
}

void tft_draw_number(uint16_t x, uint16_t y, float number, uint8_t decimals, uint16_t color, uint8_t size) {
  char buffer[20];
  sprintf(buffer, "%.*f", decimals, number);
  
  int pos = 0;
  for (int i = 0; buffer[i] != '\0'; i++) {
    if (buffer[i] >= '0' && buffer[i] <= '9') {
      tft_draw_digit(x + pos * 6 * size, y, buffer[i] - '0', color, size);
      pos++;
    } else if (buffer[i] == '.') {
      tft_fill_rect(x + pos * 6 * size, y + 6 * size, size, size, color);
      pos++;
    }
  }
}

void tft_draw_string(uint16_t x, uint16_t y, const char* str, uint16_t color, uint8_t size) {
  // Simple text drawing - just for labels
  int pos = 0;
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] >= 'A' && str[i] <= 'Z') {
      // Draw simple block letters
      tft_fill_rect(x + pos * 6 * size, y, 4 * size, 7 * size, color);
      pos++;
    } else if (str[i] == ' ') {
      pos++;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=========================================");
  Serial.println("Digital Meters Display - ESP32-C3");
  Serial.println("Direct SPI Control - No External Libraries");
  Serial.println("=========================================");
  
  // Initialize pins
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_BL, OUTPUT);
  
  // Turn on backlight
  digitalWrite(TFT_BL, HIGH);
  Serial.println("Backlight ON");
  
  // Initialize SPI
  mySPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  mySPI.setFrequency(27000000);
  Serial.println("SPI initialized");
  
  // Initialize display (using our working initialization from HelloWorld.ino)
  initDisplay();
  
  // Clear screen
  tft_fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, BLACK);
  
  // Show startup message
  showStartup();
  
  Serial.println("Digital meters display ready!");
}

void initDisplay() {
  // Use the same initialization sequence that worked in HelloWorld.ino
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TFT_DC, LOW);
  
  delay(200);
  
  // GC9A01 initialization sequence (abbreviated)
  tft_send_cmd(0xEF);
  tft_send_cmd(0xEB);
  tft_send_data(0x14);
  
  tft_send_cmd(0xFE);
  tft_send_cmd(0xEF);
  
  tft_send_cmd(0x3A);
  tft_send_data(0x05);  // 16-bit color
  
  tft_send_cmd(0x11);  // Exit Sleep
  delay(120);
  
  tft_send_cmd(0x29);  // Display on
  delay(20);
  
  Serial.println("Display initialized");
}

void showStartup() {
  // Simple startup animation
  for (int i = 0; i < 240; i += 10) {
    tft_fill_rect(i, 120, 10, 2, CYAN);
    delay(20);
  }
  
  // Title
  tft_fill_rect(60, 100, 120, 40, DARK_GRAY);
  tft_draw_rect(60, 100, 120, 40, WHITE);
  
  delay(2000);
  tft_fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, BLACK);
}

void drawSensorPanel(int index, uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  if (index >= numSensors) return;
  
  SensorData &sensor = sensors[index];
  
  // Panel background
  tft_fill_rect(x, y, w, h, DARK_GRAY);
  
  // Border color based on warning status
  uint16_t borderColor = sensor.warning ? RED : sensor.color;
  tft_draw_rect(x, y, w, h, borderColor);
  
  // Status LED
  tft_fill_rect(x + w - 8, y + 3, 5, 5, borderColor);
  
  // Value display - large numbers
  tft_draw_number(x + 5, y + 10, sensor.value, 1, WHITE, 2);
  
  // Unit
  tft_fill_rect(x + 5, y + 35, 20, 8, sensor.color);
  
  // Progress bar
  uint16_t barWidth = w - 10;
  uint16_t barHeight = 6;
  uint16_t barX = x + 5;
  uint16_t barY = y + h - 12;
  
  // Bar background
  tft_fill_rect(barX, barY, barWidth, barHeight, GRAY);
  
  // Bar fill
  float percent = (sensor.value - sensor.min) / (sensor.max - sensor.min);
  if (percent > 1.0) percent = 1.0;
  if (percent < 0.0) percent = 0.0;
  
  uint16_t fillWidth = barWidth * percent;
  tft_fill_rect(barX, barY, fillWidth, barHeight, sensor.color);
}

void updateSensorValues() {
  // Simulate sensor readings with smooth animation
  for (int i = 0; i < numSensors; i++) {
    // Randomly change targets
    if (random(300) < 1) {
      switch (i) {
        case 0: // Temperature
          sensors[i].target = random(200, 450) / 10.0;
          break;
        case 1: // Voltage
          sensors[i].target = random(300, 360) / 100.0;
          break;
        case 2: // CPU
          sensors[i].target = random(200, 950) / 10.0;
          break;
        case 3: // Memory
          sensors[i].target = random(300, 850) / 10.0;
          break;
      }
    }
    
    // Smooth interpolation
    float diff = sensors[i].target - sensors[i].value;
    sensors[i].value += diff * 0.05;
    
    // Check warning conditions
    sensors[i].warning = false;
    if (i == 0 && sensors[i].value > 45) sensors[i].warning = true; // High temp
    if (i == 1 && sensors[i].value < 3.0) sensors[i].warning = true; // Low voltage
    if (i == 2 && sensors[i].value > 90) sensors[i].warning = true; // High CPU
    if (i == 3 && sensors[i].value > 85) sensors[i].warning = true; // High memory
  }
  
  // Add some real sensor influence
  sensors[0].value = 20.0 + (analogRead(A0) / 4095.0) * 30.0;  // Temperature
  sensors[1].value = 3.0 + (analogRead(A1) / 4095.0) * 0.5;    // Voltage
}

void drawStatusBar() {
  // Top status bar
  tft_fill_rect(0, 0, TFT_WIDTH, 20, DARK_GRAY);
  tft_draw_line(0, 20, TFT_WIDTH, 20, GRAY);
  
  // System info
  tft_draw_number(5, 6, ESP.getCpuFreqMHz(), 0, GREEN, 1);
  tft_draw_number(180, 6, ESP.getFreeHeap()/1024, 0, CYAN, 1);
  
  // Time
  unsigned long seconds = millis() / 1000;
  unsigned long minutes = seconds / 60;
  seconds = seconds % 60;
  
  tft_draw_number(100, 6, minutes, 0, WHITE, 1);
  tft_draw_number(120, 6, seconds, 0, WHITE, 1);
}

void drawAnimatedElements() {
  // Animated corner indicators
  animationPhase += 0.1;
  if (animationPhase > 6.28) animationPhase = 0;
  
  uint16_t brightness = 128 + 127 * sin(animationPhase);
  uint16_t animColor = (brightness >> 3) << 11 | (brightness >> 2) << 5 | (brightness >> 3);
  
  tft_fill_rect(5, 225, 10, 10, animColor);
  tft_fill_rect(225, 225, 10, 10, animColor);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Update sensor values every 100ms
  if (currentTime - lastDataUpdate > 100) {
    updateSensorValues();
    lastDataUpdate = currentTime;
  }
  
  // Update display every 200ms
  if (currentTime - lastUpdate > 200) {
    // Clear main area (keep status bar)
    tft_fill_rect(0, 25, TFT_WIDTH, TFT_HEIGHT - 25, BLACK);
    
    // Draw status bar
    drawStatusBar();
    
    // Draw sensor panels in 2x2 grid
    drawSensorPanel(0, 10, 30, 110, 50);   // Temperature
    drawSensorPanel(1, 125, 30, 110, 50);  // Voltage
    drawSensorPanel(2, 10, 90, 110, 50);   // CPU
    drawSensorPanel(3, 125, 90, 110, 50);  // Memory
    
    // Draw animated elements
    drawAnimatedElements();
    
    lastUpdate = currentTime;
  }
  
  // Serial output every 2 seconds
  static unsigned long lastSerial = 0;
  if (currentTime - lastSerial > 2000) {
    Serial.printf("Sensors - Temp: %.1f°C, Volt: %.2fV, CPU: %.1f%%, Mem: %.1f%%\n",
                  sensors[0].value, sensors[1].value, sensors[2].value, sensors[3].value);
    lastSerial = currentTime;
  }
}
