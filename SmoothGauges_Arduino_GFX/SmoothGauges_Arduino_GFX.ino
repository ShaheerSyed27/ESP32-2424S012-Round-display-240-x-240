/*
  ESP32-C3 Smooth Analog Gauge Display - Advanced Anti-Flickering Implementation
  
  This implementation demonstrates sophisticated analog gauge rendering with smooth needle
  animations and selective update techniques. Perfect for automotive dashboards, industrial
  monitoring, or any application requiring analog-style data visualization.
  
  Hardware:
  - ESP32-C3 microcontroller (RISC-V architecture)
  - GC9A01 240x240 round IPS display
  - Arduino_GFX library for optimized rendering
  
  Advanced Features:
  - 4 independent analog gauges with configurable ranges
  - Smooth needle interpolation (no jerky movements)
  - Selective needle-only updates (background preservation)
  - Center value displays with real-time updates
  - Color-coded gauge themes for different metrics
  - Memory-efficient rendering with change tracking
  
  Anti-Flickering Techniques:
  1. Gauge backgrounds drawn once during initialization
  2. Needle updates clear only the needle area
  3. Center value updates use selective text clearing
  4. Change tracking prevents unnecessary redraws
  5. Interpolated animations for smooth movement
  
  Pin Configuration (ESP32-C3 specific):
  - SCLK: GPIO 6  (SPI Clock)
  - MOSI: GPIO 7  (SPI Data Output)
  - CS:   GPIO 10 (Chip Select)
  - DC:   GPIO 2  (Data/Command)
  - RST:  Not connected (software reset)
  - BL:   GPIO 3  (Backlight control)
  
  Gauge Configurations:
  - Temperature: 0-100°C (Red theme, top position)
  - Voltage: 0-5V (Green theme, bottom position)
  - RPM: 0-200 (Blue theme, left position)
  - Fuel: 0-100% (Orange theme, right position)
  
  Author: Community Contribution
  License: MIT
  Based on: Professional automotive gauge design principles
*/

#include <Arduino_GFX_Library.h>

// Hardware Configuration - Arduino_GFX with ESP32-C3 optimized SPI
Arduino_DataBus *bus = new Arduino_ESP32SPI(
  2,                     // DC (Data/Command) pin - GPIO 2
  10,                    // CS (Chip Select) pin - GPIO 10
  6,                     // SCK (Clock) pin - GPIO 6
  7,                     // MOSI (Data) pin - GPIO 7
  GFX_NOT_DEFINED        // MISO not used for display
);

// GC9A01 Display Controller with IPS optimization
Arduino_GFX *gfx = new Arduino_GC9A01(
  bus,                   // SPI bus interface
  GFX_NOT_DEFINED,       // RST pin (not connected)
  0,                     // Rotation (0 = normal)
  true                   // IPS panel (improved color accuracy)
);

// Professional Color Palette (16-bit RGB565)
#define BLACK       0x0000  // Background color
#define WHITE       0xFFFF  // Primary text and needles
#define RED         0xF800  // Temperature gauge theme
#define GREEN       0x07E0  // Voltage gauge theme
#define BLUE        0x001F  // RPM gauge theme
#define CYAN        0x07FF  // Secondary accents
#define MAGENTA     0xF81F  // Warning indicators
#define YELLOW      0xFFE0  // Alert colors
#define ORANGE      0xFD20  // Fuel gauge theme
#define GRAY        0x8410  // Secondary text
#define DARK_GRAY   0x4208  // Background elements
#define LIGHT_GRAY  0xC618  // UI borders

/**
 * Smooth Gauge Data Structure
 * Comprehensive structure for managing analog gauge state and rendering parameters
 */
struct SmoothGauge {
  // Position and geometry
  int centerX, centerY;          // Gauge center coordinates
  int outerRadius, innerRadius;  // Gauge ring dimensions
  int startAngle, endAngle;      // Arc sweep angles (degrees)
  
  // Value management
  float minValue, maxValue;      // Gauge scale range
  float currentValue;            // Current actual value
  float lastDrawnValue;          // Last value drawn on screen
  float targetValue;             // Target value for smooth animation
  
  // Visual properties
  uint16_t gaugeColor;           // Gauge ring color
  uint16_t needleColor;          // Needle and text color
  const char* title;             // Gauge label (e.g., "TEMP")
  const char* unit;              // Unit label (e.g., "°C")
  
  // Update tracking (anti-flickering)
  bool needsUpdate;              // Background needs redraw
  bool needsNeedleUpdate;        // Needle position changed
};

/**
 * Gauge Configuration Array
 * Defines 4 gauges with different positions, ranges, and color themes
 */
SmoothGauge gauges[] = {
  // Temperature Gauge (Top center, Red theme)
  {120, 80, 60, 35, 200, 340, 0, 100, 0, -999, 25.5, RED, WHITE, "TEMP", "C", true, true},
  
  // Voltage Gauge (Bottom center, Green theme)
  {120, 160, 60, 35, 20, 160, 0, 5, 0, -999, 3.28, GREEN, WHITE, "VOLT", "V", true, true},
  
  // RPM Gauge (Left side, Blue theme)
  {70, 120, 45, 25, 135, 45, 0, 200, 0, -999, 85, BLUE, WHITE, "RPM", "", true, true},
  
  // Fuel Gauge (Right side, Orange theme)
  {170, 120, 45, 25, 135, 45, 0, 100, 0, -999, 76, ORANGE, WHITE, "FUEL", "%", true, true}
};

// Global State Management
const int numGauges = 4;
unsigned long lastUpdate = 0;        // Needle update timing
unsigned long lastValueUpdate = 0;   // Value generation timing
bool displayInitialized = false;     // Initialization state flag

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Smooth Analog Gauge Display...");
  
  // Initialize backlight
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  
  // Initialize display
  gfx->begin();
  gfx->fillScreen(BLACK);
  
  // Initialize static elements
  initializeDisplay();
  displayInitialized = true;
  
  Serial.println("Smooth gauges ready!");
}

void initializeDisplay() {
  // Draw title (static)
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(60, 20);
  gfx->println("GAUGES");
  
  // Draw all gauge backgrounds (static)
  for (int i = 0; i < numGauges; i++) {
    drawGaugeBackground(gauges[i]);
  }
  
  // Draw status info (static)
  gfx->setTextColor(LIGHT_GRAY);
  gfx->setTextSize(1);
  gfx->setCursor(10, 225);
  gfx->println("Smooth Mode");
}

void drawGaugeBackground(SmoothGauge &gauge) {
  int cx = gauge.centerX;
  int cy = gauge.centerY;
  int outerR = gauge.outerRadius;
  
  // Draw gauge arc background
  drawArc(cx, cy, outerR, gauge.startAngle, gauge.endAngle, DARK_GRAY, 3);
  
  // Draw tick marks (static)
  float angleRange = gauge.endAngle - gauge.startAngle;
  if (angleRange < 0) angleRange += 360;
  
  for (int i = 0; i <= 10; i++) {
    float tickAngle = gauge.startAngle + (i * angleRange / 10);
    float tickRad = tickAngle * PI / 180;
    
    int x1 = cx + (outerR - 6) * cos(tickRad);
    int y1 = cy + (outerR - 6) * sin(tickRad);
    int x2 = cx + (outerR - 2) * cos(tickRad);
    int y2 = cy + (outerR - 2) * sin(tickRad);
    
    gfx->drawLine(x1, y1, x2, y2, WHITE);
  }
  
  // Draw title (static)
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  int titleLen = strlen(gauge.title) * 6;
  gfx->setCursor(cx - titleLen/2, cy + outerR + 8);
  gfx->println(gauge.title);
}

void drawArc(int cx, int cy, int radius, float startAngle, float endAngle, uint16_t color, int thickness) {
  if (endAngle < startAngle) endAngle += 360;
  
  for (int t = 0; t < thickness; t++) {
    for (float angle = startAngle; angle <= endAngle; angle += 2) {
      float rad = angle * PI / 180;
      int x = cx + (radius - t) * cos(rad);
      int y = cy + (radius - t) * sin(rad);
      gfx->drawPixel(x, y, color);
    }
  }
}

void updateGaugeValue(int index) {
  SmoothGauge &gauge = gauges[index];
  
  if (!gauge.needsUpdate) return;
  
  int cx = gauge.centerX;
  int cy = gauge.centerY;
  int outerR = gauge.outerRadius;
  int innerR = gauge.innerRadius;
  
  // Clear the old value arc area by redrawing background
  drawArc(cx, cy, outerR, gauge.startAngle, gauge.endAngle, DARK_GRAY, 3);
  
  // Calculate new value position
  float range = gauge.maxValue - gauge.minValue;
  float angleRange = gauge.endAngle - gauge.startAngle;
  if (angleRange < 0) angleRange += 360;
  
  float valuePercent = (gauge.currentValue - gauge.minValue) / range;
  float valueAngle = gauge.startAngle + (valuePercent * angleRange);
  
  // Draw new colored value arc
  drawArc(cx, cy, outerR, gauge.startAngle, valueAngle, gauge.gaugeColor, 3);
  
  gauge.needsUpdate = false;
}

void updateGaugeNeedle(int index) {
  SmoothGauge &gauge = gauges[index];
  
  if (!gauge.needsNeedleUpdate) return;
  
  int cx = gauge.centerX;
  int cy = gauge.centerY;
  int outerR = gauge.outerRadius;
  int innerR = gauge.innerRadius;
  
  // Clear needle area
  gfx->fillCircle(cx, cy, innerR, BLACK);
  
  // Calculate needle position
  float range = gauge.maxValue - gauge.minValue;
  float angleRange = gauge.endAngle - gauge.startAngle;
  if (angleRange < 0) angleRange += 360;
  
  float valuePercent = (gauge.currentValue - gauge.minValue) / range;
  float needleAngle = gauge.startAngle + (valuePercent * angleRange);
  float needleRad = needleAngle * PI / 180;
  
  // Draw needle
  int needleEndX = cx + (outerR - 15) * cos(needleRad);
  int needleEndY = cy + (outerR - 15) * sin(needleRad);
  
  gfx->drawLine(cx, cy, needleEndX, needleEndY, gauge.needleColor);
  gfx->fillCircle(cx, cy, 4, gauge.gaugeColor);
  gfx->fillCircle(cx, cy, 2, BLACK);
  
  // Draw value in center
  gfx->setTextColor(gauge.gaugeColor);
  gfx->setTextSize(1);
  
  char valueStr[15];
  if (strlen(gauge.unit) > 0) {
    sprintf(valueStr, "%.1f%s", gauge.currentValue, gauge.unit);
  } else {
    sprintf(valueStr, "%.0f", gauge.currentValue);
  }
  
  int valueLen = strlen(valueStr) * 6;
  gfx->setCursor(cx - valueLen/2, cy + 8);
  gfx->println(valueStr);
  
  gauge.needsNeedleUpdate = false;
}

void updateValues() {
  // Simulate sensor readings
  for (int i = 0; i < numGauges; i++) {
    // Change target values occasionally
    if (random(300) < 1) {
      switch (i) {
        case 0: // Temperature
          gauges[i].targetValue = random(200, 500) / 10.0;
          break;
        case 1: // Voltage
          gauges[i].targetValue = random(280, 380) / 100.0;
          break;
        case 2: // RPM
          gauges[i].targetValue = random(500, 1800) / 10.0;
          break;
        case 3: // Fuel
          gauges[i].targetValue = random(300, 950) / 10.0;
          break;
      }
    }
    
    // Smooth interpolation
    float diff = gauges[i].targetValue - gauges[i].currentValue;
    gauges[i].currentValue += diff * 0.05;
    
    // Check if significant change occurred
    float changeDiff = abs(gauges[i].currentValue - gauges[i].lastDrawnValue);
    if (changeDiff > 1.0) {
      gauges[i].needsUpdate = true;
      gauges[i].needsNeedleUpdate = true;
      gauges[i].lastDrawnValue = gauges[i].currentValue;
    }
  }
  
  // Add some real sensor influence
  gauges[0].currentValue = 20.0 + (analogRead(A0) / 4095.0) * 30.0;
  gauges[1].currentValue = 3.0 + (analogRead(A1) / 4095.0) * 0.6;
}

void loop() {
  unsigned long currentTime = millis();
  
  if (!displayInitialized) return;
  
  // Update values every 100ms
  if (currentTime - lastValueUpdate > 100) {
    updateValues();
    lastValueUpdate = currentTime;
  }
  
  // Update display every 50ms (only changed parts)
  if (currentTime - lastUpdate > 50) {
    for (int i = 0; i < numGauges; i++) {
      updateGaugeValue(i);
      updateGaugeNeedle(i);
    }
    lastUpdate = currentTime;
  }
  
  // Serial output every 2 seconds
  static unsigned long lastSerial = 0;
  if (currentTime - lastSerial > 2000) {
    Serial.printf("Gauges - Temp: %.1f°C, Volt: %.2fV, RPM: %.0f, Fuel: %.0f%%\n",
                  gauges[0].currentValue, gauges[1].currentValue,
                  gauges[2].currentValue, gauges[3].currentValue);
    lastSerial = currentTime;
  }
}
