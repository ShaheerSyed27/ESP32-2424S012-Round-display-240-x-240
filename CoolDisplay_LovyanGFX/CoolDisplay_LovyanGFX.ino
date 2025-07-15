/*
  Cool Digital Display with Meters and Dials for ESP32-C3 with GC9A01
  Using LovyanGFX library as a replacement for TFT_eSPI
  Features: Analog gauges, digital meters, temperature display, voltage meter
*/

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// Custom LGFX class for ESP32-C3 with GC9A01
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_GC9A01 _panel_instance;
  lgfx::Bus_SPI _bus_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST;  // Use FSPI for ESP32-C3
      cfg.spi_mode = 0;
      cfg.freq_write = 27000000; // 27MHz
      cfg.freq_read = 16000000;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 6;   // SPI Clock
      cfg.pin_mosi = 7;   // SPI Data Output (MOSI)
      cfg.pin_miso = -1;  // Not used
      cfg.pin_dc = 2;     // Data/Command select
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs = 10;    // Chip Select
      cfg.pin_rst = -1;   // Reset pin (not connected)
      cfg.pin_busy = -1;  // Not used
      cfg.memory_width = 240;
      cfg.memory_height = 240;
      cfg.panel_width = 240;
      cfg.panel_height = 240;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = true;
      cfg.invert = false;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;
      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);
  }
};

static LGFX lcd;
static LGFX_Sprite sprite(&lcd);  // Sprite for smooth animations

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
#define LIGHT_GRAY  0xBDF7

// Gauge parameters
struct Gauge {
  int centerX, centerY;
  int radius;
  int startAngle, endAngle;
  float minValue, maxValue;
  float currentValue;
  uint16_t color;
  const char* label;
  const char* unit;
};

// Initialize gauges
Gauge tempGauge = {120, 80, 60, 135, 45, 0, 100, 23.5, RED, "TEMP", "°C"};
Gauge voltGauge = {120, 180, 60, 225, 315, 0, 5, 3.3, GREEN, "VOLT", "V"};
Gauge speedGauge = {60, 120, 50, 180, 0, 0, 200, 85, BLUE, "RPM", "x100"};
Gauge battGauge = {180, 120, 50, 0, 180, 0, 100, 78, ORANGE, "BATT", "%"};

// Animation variables
float animationStep = 0;
unsigned long lastUpdate = 0;
unsigned long lastDataUpdate = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=========================================");
  Serial.println("Cool Digital Display - ESP32-C3 + GC9A01");
  Serial.println("Using LovyanGFX Library");
  Serial.println("=========================================");
  
  // Initialize backlight
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  Serial.println("Backlight ON");
  
  // Initialize display
  lcd.init();
  lcd.setRotation(0);
  lcd.fillScreen(BLACK);
  
  // Show startup screen
  showStartupScreen();
  delay(2000);
  
  // Initialize sprite for smooth animations
  sprite.createSprite(240, 240);
  sprite.setColorDepth(16);
  
  Serial.println("Display initialized successfully!");
}

void showStartupScreen() {
  lcd.fillScreen(BLACK);
  
  // Draw title
  lcd.setTextColor(WHITE);
  lcd.setTextSize(2);
  lcd.setCursor(60, 50);
  lcd.println("DIGITAL");
  lcd.setCursor(80, 75);
  lcd.println("DASH");
  
  // Draw subtitle
  lcd.setTextColor(GRAY);
  lcd.setTextSize(1);
  lcd.setCursor(70, 110);
  lcd.println("ESP32-C3 + GC9A01");
  lcd.setCursor(85, 125);
  lcd.println("LovyanGFX");
  
  // Draw loading bar
  int barWidth = 160;
  int barHeight = 8;
  int barX = (240 - barWidth) / 2;
  int barY = 160;
  
  lcd.drawRect(barX - 2, barY - 2, barWidth + 4, barHeight + 4, WHITE);
  
  for (int i = 0; i <= barWidth; i += 4) {
    lcd.fillRect(barX, barY, i, barHeight, GREEN);
    delay(20);
  }
  
  // Version info
  lcd.setTextColor(DARK_GRAY);
  lcd.setCursor(90, 200);
  lcd.println("v1.0.0");
}

void drawGauge(Gauge& gauge) {
  int cx = gauge.centerX;
  int cy = gauge.centerY;
  int radius = gauge.radius;
  
  // Draw gauge background arc
  for (int r = radius - 8; r <= radius; r++) {
    drawArc(cx, cy, r, gauge.startAngle, gauge.endAngle, DARK_GRAY);
  }
  
  // Calculate needle angle
  float valueRange = gauge.maxValue - gauge.minValue;
  float angleRange = gauge.endAngle - gauge.startAngle;
  if (angleRange < 0) angleRange += 360;
  
  float valuePercent = (gauge.currentValue - gauge.minValue) / valueRange;
  float needleAngle = gauge.startAngle + (valuePercent * angleRange);
  
  // Draw value arc (colored portion)
  for (int r = radius - 8; r <= radius; r++) {
    drawArc(cx, cy, r, gauge.startAngle, needleAngle, gauge.color);
  }
  
  // Draw center circle
  sprite.fillCircle(cx, cy, 8, WHITE);
  sprite.fillCircle(cx, cy, 6, BLACK);
  
  // Draw needle
  float needleRadians = needleAngle * PI / 180.0;
  int needleEndX = cx + (radius - 15) * cos(needleRadians);
  int needleEndY = cy + (radius - 15) * sin(needleRadians);
  
  sprite.drawLine(cx, cy, needleEndX, needleEndY, WHITE);
  sprite.fillCircle(cx, cy, 3, gauge.color);
  
  // Draw label and value
  sprite.setTextColor(WHITE);
  sprite.setTextSize(1);
  
  // Label
  int labelWidth = strlen(gauge.label) * 6;
  sprite.setCursor(cx - labelWidth/2, cy + radius + 10);
  sprite.println(gauge.label);
  
  // Value
  char valueStr[20];
  sprintf(valueStr, "%.1f%s", gauge.currentValue, gauge.unit);
  int valueWidth = strlen(valueStr) * 6;
  sprite.setCursor(cx - valueWidth/2, cy + radius + 25);
  sprite.setTextColor(gauge.color);
  sprite.println(valueStr);
}

void drawArc(int cx, int cy, int radius, float startAngle, float endAngle, uint16_t color) {
  if (endAngle < startAngle) endAngle += 360;
  
  for (float angle = startAngle; angle <= endAngle; angle += 2) {
    float radians = angle * PI / 180.0;
    int x = cx + radius * cos(radians);
    int y = cy + radius * sin(radians);
    sprite.drawPixel(x, y, color);
  }
}

void drawDigitalReadouts() {
  // System info panel
  sprite.drawRect(10, 10, 220, 40, GRAY);
  sprite.setTextColor(WHITE);
  sprite.setTextSize(1);
  sprite.setCursor(15, 20);
  sprite.println("ESP32-C3 Digital Dashboard");
  
  sprite.setTextColor(GREEN);
  sprite.setCursor(15, 35);
  sprite.printf("CPU: %dMHz  RAM: %dKB", ESP.getCpuFreqMHz(), ESP.getFreeHeap()/1024);
  
  // Time display
  sprite.setTextColor(CYAN);
  sprite.setTextSize(2);
  sprite.setCursor(80, 200);
  unsigned long seconds = millis() / 1000;
  unsigned long minutes = seconds / 60;
  seconds = seconds % 60;
  sprite.printf("%02lu:%02lu", minutes, seconds);
}

void updateSensorValues() {
  // Simulate sensor readings with smooth changes
  static float tempTarget = 25.0;
  static float voltTarget = 3.3;
  static float speedTarget = 90.0;
  static float battTarget = 80.0;
  
  // Randomly change targets occasionally
  if (random(100) < 2) tempTarget = random(200, 450) / 10.0;
  if (random(100) < 2) voltTarget = random(280, 360) / 100.0;
  if (random(100) < 2) speedTarget = random(300, 1800) / 10.0;
  if (random(100) < 2) battTarget = random(650, 980) / 10.0;
  
  // Smooth interpolation to targets
  tempGauge.currentValue += (tempTarget - tempGauge.currentValue) * 0.05;
  voltGauge.currentValue += (voltTarget - voltGauge.currentValue) * 0.05;
  speedGauge.currentValue += (speedTarget - speedGauge.currentValue) * 0.05;
  battGauge.currentValue += (battTarget - battGauge.currentValue) * 0.05;
  
  // Add some real sensor readings
  tempGauge.currentValue = 20.0 + (analogRead(A0) / 4095.0) * 40.0; // Simulate temperature
  voltGauge.currentValue = 3.0 + (analogRead(A1) / 4095.0) * 0.6;   // Simulate voltage
}

void drawStatusLEDs() {
  // Status indicators
  sprite.fillCircle(20, 220, 5, tempGauge.currentValue > 35 ? RED : GREEN);  // Temperature warning
  sprite.fillCircle(40, 220, 5, voltGauge.currentValue < 3.0 ? RED : GREEN); // Voltage warning
  sprite.fillCircle(60, 220, 5, battGauge.currentValue < 20 ? RED : GREEN);  // Battery warning
  sprite.fillCircle(80, 220, 5, GREEN);  // System OK
  
  // Status labels
  sprite.setTextColor(WHITE);
  sprite.setTextSize(1);
  sprite.setCursor(15, 200);
  sprite.println("T V B S");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Update sensor values every 100ms
  if (currentTime - lastDataUpdate > 100) {
    updateSensorValues();
    lastDataUpdate = currentTime;
  }
  
  // Update display every 50ms for smooth animation
  if (currentTime - lastUpdate > 50) {
    // Clear sprite
    sprite.fillSprite(BLACK);
    
    // Draw all components
    drawDigitalReadouts();
    drawGauge(tempGauge);
    drawGauge(voltGauge);
    drawGauge(speedGauge);
    drawGauge(battGauge);
    drawStatusLEDs();
    
    // Add animated border
    animationStep += 0.1;
    if (animationStep > 360) animationStep = 0;
    
    uint16_t borderColor = sprite.color565(
      128 + 127 * sin(animationStep * PI / 180),
      128 + 127 * sin((animationStep + 120) * PI / 180),
      128 + 127 * sin((animationStep + 240) * PI / 180)
    );
    
    // Draw animated border
    sprite.drawRect(0, 0, 240, 240, borderColor);
    sprite.drawRect(1, 1, 238, 238, borderColor);
    
    // Push sprite to display
    sprite.pushSprite(0, 0);
    
    lastUpdate = currentTime;
  }
  
  // Print debug info occasionally
  static unsigned long lastSerial = 0;
  if (currentTime - lastSerial > 2000) {
    Serial.printf("Temp: %.1f°C, Volt: %.2fV, Speed: %.0f RPM, Batt: %.0f%%\n", 
                  tempGauge.currentValue, voltGauge.currentValue, 
                  speedGauge.currentValue, battGauge.currentValue);
    lastSerial = currentTime;
  }
}
