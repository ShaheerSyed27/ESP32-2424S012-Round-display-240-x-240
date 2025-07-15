/*
  Analog Gauge Display using Arduino_GFX Library
  ESP32-C3 with GC9A01 Round Display
  Features multiple analog gauges with smooth animations
*/

#include <Arduino_GFX_Library.h>

// Hardware configuration for ESP32-C3 with GC9A01
Arduino_DataBus *bus = new Arduino_ESP32SPI(2 /* DC */, 10 /* CS */, 6 /* SCK */, 7 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);

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

// Gauge structure
struct AnalogGauge {
  int centerX, centerY;
  int outerRadius, innerRadius;
  int startAngle, endAngle;  // in degrees
  float minValue, maxValue;
  float currentValue, targetValue;
  uint16_t gaugeColor, needleColor;
  const char* title;
  const char* unit;
};

// Define multiple gauges
AnalogGauge gauges[] = {
  {120, 80, 65, 45, 200, 340, 0, 100, 25.5, 30.0, RED, WHITE, "TEMP", "C"},      // Temperature
  {120, 160, 65, 45, 20, 160, 0, 5, 3.28, 3.5, GREEN, WHITE, "VOLT", "V"},       // Voltage  
  {70, 120, 50, 35, 135, 45, 0, 200, 85, 95, BLUE, WHITE, "RPM", ""},            // RPM
  {170, 120, 50, 35, 135, 45, 0, 100, 76, 82, ORANGE, WHITE, "FUEL", "%"}        // Fuel
};

const int numGauges = sizeof(gauges) / sizeof(gauges[0]);

// Animation and timing
unsigned long lastUpdate = 0;
unsigned long lastValueUpdate = 0;
float animationPhase = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino_GFX Analog Gauge Display...");
  
  // Initialize backlight
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  
  // Initialize display
  gfx->begin();
  gfx->fillScreen(BLACK);
  
  // Show startup animation
  showStartup();
  
  Serial.println("Display initialized successfully!");
}

void showStartup() {
  gfx->fillScreen(BLACK);
  
  // Title
  gfx->setTextColor(WHITE);
  gfx->setTextSize(3);
  gfx->setCursor(60, 100);
  gfx->println("GAUGE");
  gfx->setCursor(50, 130);
  gfx->println("DISPLAY");
  
  // Animated loading circle
  for (int i = 0; i < 360; i += 10) {
    float rad = i * PI / 180;
    int x = 120 + 80 * cos(rad);
    int y = 120 + 80 * sin(rad);
    gfx->fillCircle(x, y, 3, CYAN);
    delay(50);
  }
  
  delay(1000);
  gfx->fillScreen(BLACK);
}

void drawGauge(AnalogGauge &gauge) {
  int cx = gauge.centerX;
  int cy = gauge.centerY;
  int outerR = gauge.outerRadius;
  int innerR = gauge.innerRadius;
  
  // Draw gauge background arc
  drawArc(cx, cy, outerR, gauge.startAngle, gauge.endAngle, DARK_GRAY, 4);
  
  // Calculate value position
  float range = gauge.maxValue - gauge.minValue;
  float angleRange = gauge.endAngle - gauge.startAngle;
  if (angleRange < 0) angleRange += 360;
  
  float valuePercent = (gauge.currentValue - gauge.minValue) / range;
  float valueAngle = gauge.startAngle + (valuePercent * angleRange);
  
  // Draw colored value arc
  drawArc(cx, cy, outerR, gauge.startAngle, valueAngle, gauge.gaugeColor, 4);
  
  // Draw tick marks
  for (int i = 0; i <= 10; i++) {
    float tickAngle = gauge.startAngle + (i * angleRange / 10);
    float tickRad = tickAngle * PI / 180;
    
    int x1 = cx + (outerR - 8) * cos(tickRad);
    int y1 = cy + (outerR - 8) * sin(tickRad);
    int x2 = cx + (outerR - 2) * cos(tickRad);
    int y2 = cy + (outerR - 2) * sin(tickRad);
    
    gfx->drawLine(x1, y1, x2, y2, WHITE);
  }
  
  // Draw needle
  float needleRad = valueAngle * PI / 180;
  int needleEndX = cx + (outerR - 12) * cos(needleRad);
  int needleEndY = cy + (outerR - 12) * sin(needleRad);
  
  gfx->drawLine(cx, cy, needleEndX, needleEndY, gauge.needleColor);
  gfx->fillCircle(cx, cy, 5, gauge.gaugeColor);
  gfx->fillCircle(cx, cy, 3, BLACK);
  
  // Draw title
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  int titleLen = strlen(gauge.title) * 6;
  gfx->setCursor(cx - titleLen/2, cy + outerR + 5);
  gfx->println(gauge.title);
  
  // Draw value
  char valueStr[20];
  if (strlen(gauge.unit) > 0) {
    sprintf(valueStr, "%.1f%s", gauge.currentValue, gauge.unit);
  } else {
    sprintf(valueStr, "%.0f", gauge.currentValue);
  }
  
  gfx->setTextColor(gauge.gaugeColor);
  int valueLen = strlen(valueStr) * 6;
  gfx->setCursor(cx - valueLen/2, cy + outerR + 18);
  gfx->println(valueStr);
}

void drawArc(int cx, int cy, int radius, float startAngle, float endAngle, uint16_t color, int thickness) {
  if (endAngle < startAngle) endAngle += 360;
  
  for (int t = 0; t < thickness; t++) {
    for (float angle = startAngle; angle <= endAngle; angle += 1) {
      float rad = angle * PI / 180;
      int x = cx + (radius - t) * cos(rad);
      int y = cy + (radius - t) * sin(rad);
      gfx->drawPixel(x, y, color);
    }
  }
}

void updateValues() {
  // Simulate sensor readings and smooth animation
  for (int i = 0; i < numGauges; i++) {
    // Randomly change target values occasionally
    if (random(200) < 1) {
      switch (i) {
        case 0: // Temperature
          gauges[i].targetValue = random(150, 450) / 10.0;
          break;
        case 1: // Voltage
          gauges[i].targetValue = random(280, 380) / 100.0;
          break;
        case 2: // RPM
          gauges[i].targetValue = random(300, 1800) / 10.0;
          break;
        case 3: // Fuel
          gauges[i].targetValue = random(200, 950) / 10.0;
          break;
      }
    }
    
    // Smooth interpolation to target
    float diff = gauges[i].targetValue - gauges[i].currentValue;
    gauges[i].currentValue += diff * 0.03;
  }
  
  // Add some real sensor influence
  gauges[0].currentValue = 20.0 + (analogRead(A0) / 4095.0) * 30.0;  // Temperature simulation
  gauges[1].currentValue = 3.0 + (analogRead(A1) / 4095.0) * 0.6;    // Voltage simulation
}

void drawStatusPanel() {
  // System status panel at bottom
  gfx->drawRect(10, 200, 220, 35, GRAY);
  
  gfx->setTextColor(WHITE);
  gfx->setTextSize(1);
  gfx->setCursor(15, 210);
  gfx->println("ESP32-C3 Status:");
  
  gfx->setTextColor(GREEN);
  gfx->setCursor(15, 225);
  gfx->printf("CPU:%dMHz RAM:%dKB", ESP.getCpuFreqMHz(), ESP.getFreeHeap()/1024);
  
  // Running time
  unsigned long seconds = millis() / 1000;
  unsigned long minutes = seconds / 60;
  seconds = seconds % 60;
  
  gfx->setTextColor(CYAN);
  gfx->setCursor(160, 225);
  gfx->printf("%02lu:%02lu", minutes, seconds);
}

void drawAnimatedBorder() {
  // Animated rainbow border
  animationPhase += 2;
  if (animationPhase > 360) animationPhase = 0;
  
  uint16_t borderColor = gfx->color565(
    128 + 127 * sin(animationPhase * PI / 180),
    128 + 127 * sin((animationPhase + 120) * PI / 180),
    128 + 127 * sin((animationPhase + 240) * PI / 180)
  );
  
  // Draw corner accents
  gfx->fillCircle(10, 10, 5, borderColor);
  gfx->fillCircle(230, 10, 5, borderColor);
  gfx->fillCircle(10, 230, 5, borderColor);
  gfx->fillCircle(230, 230, 5, borderColor);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Update values every 100ms
  if (currentTime - lastValueUpdate > 100) {
    updateValues();
    lastValueUpdate = currentTime;
  }
  
  // Update display every 50ms for smooth animation
  if (currentTime - lastUpdate > 50) {
    gfx->fillScreen(BLACK);
    
    // Draw all gauges
    for (int i = 0; i < numGauges; i++) {
      drawGauge(gauges[i]);
    }
    
    // Draw additional elements
    drawStatusPanel();
    drawAnimatedBorder();
    
    lastUpdate = currentTime;
  }
  
  // Serial output every 2 seconds
  static unsigned long lastSerial = 0;
  if (currentTime - lastSerial > 2000) {
    Serial.printf("Temp: %.1f°C, Volt: %.2fV, RPM: %.0f, Fuel: %.0f%%\n",
                  gauges[0].currentValue, gauges[1].currentValue,
                  gauges[2].currentValue, gauges[3].currentValue);
    lastSerial = currentTime;
  }
}
