/*
  Smooth Analog Gauge Display - No Flicker
  ESP32-C3 with GC9A01 Round Display
  Features: Analog gauges with smooth updates and center value display
*/

#include <Arduino_GFX_Library.h>

// Hardware configuration
Arduino_DataBus *bus = new Arduino_ESP32SPI(2 /* DC */, 10 /* CS */, 6 /* SCK */, 7 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);

// Color definitions
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

// Gauge structure
struct SmoothGauge {
  int centerX, centerY;
  int outerRadius, innerRadius;
  int startAngle, endAngle;
  float minValue, maxValue;
  float currentValue, lastDrawnValue;
  float targetValue;
  uint16_t gaugeColor, needleColor;
  const char* title;
  const char* unit;
  bool needsUpdate;
  bool needsNeedleUpdate;
};

// Define gauges
SmoothGauge gauges[] = {
  {120, 80, 60, 35, 200, 340, 0, 100, 0, -999, 25.5, RED, WHITE, "TEMP", "C", true, true},
  {120, 160, 60, 35, 20, 160, 0, 5, 0, -999, 3.28, GREEN, WHITE, "VOLT", "V", true, true},
  {70, 120, 45, 25, 135, 45, 0, 200, 0, -999, 85, BLUE, WHITE, "RPM", "", true, true},
  {170, 120, 45, 25, 135, 45, 0, 100, 0, -999, 76, ORANGE, WHITE, "FUEL", "%", true, true}
};

const int numGauges = 4;
unsigned long lastUpdate = 0;
unsigned long lastValueUpdate = 0;
bool displayInitialized = false;

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
