/*
  Modern Digital Dashboard for ESP32-C3 with GC9A01
  Using Arduino_GFX library with sleek modern design
  Features: Digital meters, progress bars, real-time graphs
*/

#include <Arduino_GFX_Library.h>

// Hardware configuration
Arduino_DataBus *bus = new Arduino_ESP32SPI(2 /* DC */, 10 /* CS */, 6 /* SCK */, 7 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, true /* IPS */);

// Modern color palette
#define BG_COLOR        0x0841  // Dark blue-gray
#define PANEL_COLOR     0x1082  // Lighter panel color
#define ACCENT_COLOR    0x051D  // Bright blue accent
#define SUCCESS_COLOR   0x07E0  // Green
#define WARNING_COLOR   0xFFE0  // Yellow
#define DANGER_COLOR    0xF800  // Red
#define TEXT_PRIMARY    0xFFFF  // White
#define TEXT_SECONDARY  0x8410  // Gray

// Data structure for metrics
struct Metric {
  float value;
  float target;
  float min, max;
  const char* label;
  const char* unit;
  uint16_t color;
  float dangerThreshold;
  float warningThreshold;
};

// Define system metrics
Metric metrics[] = {
  {0, 0, 0, 100, "CPU", "%", ACCENT_COLOR, 90, 70},
  {0, 0, 0, 100, "MEM", "%", SUCCESS_COLOR, 85, 65},
  {0, 0, 0, 60, "TEMP", "C", WARNING_COLOR, 50, 35},
  {0, 0, 0, 5, "VOLT", "V", ACCENT_COLOR, 2.8f, 3.0f}
};

const int numMetrics = sizeof(metrics) / sizeof(metrics[0]);

// Graph data
float graphData[4][60];  // 60 data points for each metric
int graphIndex = 0;

// Animation and timing
unsigned long lastUpdate = 0;
unsigned long lastDataUpdate = 0;
unsigned long lastGraphUpdate = 0;
unsigned long lastHeaderUpdate = 0;
float animationTime = 0;
bool firstDraw = true;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Modern Digital Dashboard...");
  
  // Initialize backlight
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  
  // Initialize display
  gfx->begin();
  gfx->fillScreen(BG_COLOR);
  
  // Initialize graph data
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 60; j++) {
      graphData[i][j] = 0;
    }
  }
  
  showBootScreen();
  Serial.println("Dashboard initialized!");
}

void showBootScreen() {
  gfx->fillScreen(BG_COLOR);
  
  // Animated startup sequence
  for (int i = 0; i < 240; i += 8) {
    gfx->drawFastHLine(0, 120, i, ACCENT_COLOR);
    delay(10);
  }
  
  // Logo/Title
  gfx->setTextColor(TEXT_PRIMARY);
  gfx->setTextSize(2);
  gfx->setCursor(60, 100);
  gfx->println("DIGITAL");
  gfx->setCursor(45, 125);
  gfx->println("DASHBOARD");
  
  // Version
  gfx->setTextColor(TEXT_SECONDARY);
  gfx->setTextSize(1);
  gfx->setCursor(95, 155);
  gfx->println("v2.0");
  
  delay(2000);
}

void updateMetrics() {
  // Simulate realistic system metrics
  static unsigned long lastChange = 0;
  
  if (millis() - lastChange > 1000) {
    // CPU usage simulation
    metrics[0].target = 20 + random(0, 60);
    
    // Memory usage simulation  
    metrics[1].target = 30 + random(0, 40);
    
    // Temperature simulation (influenced by CPU load)
    metrics[2].target = 25 + (metrics[0].value * 0.3) + random(-5, 10);
    
    // Voltage simulation
    metrics[3].target = 3.2 + random(-20, 20) / 100.0;
    
    lastChange = millis();
  }
  
  // Smooth interpolation
  for (int i = 0; i < numMetrics; i++) {
    float diff = metrics[i].target - metrics[i].value;
    metrics[i].value += diff * 0.1;
    
    // Clamp to min/max
    if (metrics[i].value < metrics[i].min) metrics[i].value = metrics[i].min;
    if (metrics[i].value > metrics[i].max) metrics[i].value = metrics[i].max;
  }
  
  // Real sensor data overlay
  metrics[2].value = 20 + (analogRead(A0) / 4095.0) * 25; // Temperature
  metrics[3].value = 3.0 + (analogRead(A1) / 4095.0) * 0.5; // Voltage
}

void updateGraphData() {
  // Add current values to graph data
  for (int i = 0; i < numMetrics; i++) {
    graphData[i][graphIndex] = metrics[i].value;
  }
  
  graphIndex = (graphIndex + 1) % 60;
}

void drawMetricPanel(int x, int y, int w, int h, Metric &metric) {
  // Determine status color
  uint16_t statusColor = metric.color;
  if (metric.value > metric.dangerThreshold) {
    statusColor = DANGER_COLOR;
  } else if (metric.value > metric.warningThreshold) {
    statusColor = WARNING_COLOR;
  }
  
  // Panel background
  gfx->fillRoundRect(x, y, w, h, 8, PANEL_COLOR);
  gfx->drawRoundRect(x, y, w, h, 8, statusColor);
  
  // Status LED
  gfx->fillCircle(x + w - 8, y + 8, 3, statusColor);
  
  // Label
  gfx->setTextColor(TEXT_SECONDARY);
  gfx->setTextSize(1);
  gfx->setCursor(x + 5, y + 5);
  gfx->println(metric.label);
  
  // Value
  gfx->setTextColor(TEXT_PRIMARY);
  gfx->setTextSize(2);
  gfx->setCursor(x + 5, y + 18);
  
  char valueStr[20];
  sprintf(valueStr, "%.1f", metric.value);
  gfx->println(valueStr);
  
  // Unit
  gfx->setTextColor(TEXT_SECONDARY);
  gfx->setTextSize(1);
  gfx->setCursor(x + 5, y + 38);
  gfx->println(metric.unit);
  
  // Progress bar
  int barWidth = w - 10;
  int barHeight = 4;
  int barX = x + 5;
  int barY = y + h - 10;
  
  gfx->drawRect(barX, barY, barWidth, barHeight, TEXT_SECONDARY);
  
  float percent = (metric.value - metric.min) / (metric.max - metric.min);
  int fillWidth = barWidth * percent;
  gfx->fillRect(barX + 1, barY + 1, fillWidth - 1, barHeight - 2, statusColor);
}

void drawMiniGraph(int x, int y, int w, int h, int metricIndex) {
  gfx->drawRect(x, y, w, h, TEXT_SECONDARY);
  
  if (metricIndex >= numMetrics) return;
  
  Metric &metric = metrics[metricIndex];
  uint16_t graphColor = metric.color;
  
  // Draw graph lines
  for (int i = 1; i < 60; i++) {
    int prevIndex = (graphIndex + i - 1) % 60;
    int currIndex = (graphIndex + i) % 60;
    
    float prevValue = graphData[metricIndex][prevIndex];
    float currValue = graphData[metricIndex][currIndex];
    
    int prevY = y + h - 1 - ((prevValue - metric.min) / (metric.max - metric.min)) * (h - 2);
    int currY = y + h - 1 - ((currValue - metric.min) / (metric.max - metric.min)) * (h - 2);
    
    int prevX = x + 1 + (i - 1) * (w - 2) / 59;
    int currX = x + 1 + i * (w - 2) / 59;
    
    gfx->drawLine(prevX, prevY, currX, currY, graphColor);
  }
}

void drawHeader() {
  // Header bar
  gfx->fillRect(0, 0, 240, 25, PANEL_COLOR);
  gfx->drawFastHLine(0, 25, 240, ACCENT_COLOR);
  
  // Title
  gfx->setTextColor(TEXT_PRIMARY);
  gfx->setTextSize(1);
  gfx->setCursor(5, 10);
  gfx->println("ESP32-C3 DASHBOARD");
  
  // Time
  unsigned long seconds = millis() / 1000;
  unsigned long minutes = seconds / 60;
  seconds = seconds % 60;
  
  gfx->setTextColor(ACCENT_COLOR);
  gfx->setCursor(180, 10);
  gfx->printf("%02lu:%02lu", minutes, seconds);
  
  // WiFi/Status icon (simulated)
  gfx->fillRect(215, 8, 3, 2, SUCCESS_COLOR);
  gfx->fillRect(219, 6, 3, 4, SUCCESS_COLOR);
  gfx->fillRect(223, 4, 3, 6, SUCCESS_COLOR);
}

void drawFloatingActionButton() {
  // Animated floating button
  animationTime += 0.05;
  int bobOffset = 2 * sin(animationTime);
  
  int buttonX = 200;
  int buttonY = 180 + bobOffset;
  
  gfx->fillCircle(buttonX, buttonY, 15, ACCENT_COLOR);
  gfx->fillCircle(buttonX, buttonY, 12, PANEL_COLOR);
  
  // Plus icon
  gfx->drawFastHLine(buttonX - 5, buttonY, 10, TEXT_PRIMARY);
  gfx->drawFastVLine(buttonX, buttonY - 5, 10, TEXT_PRIMARY);
}

void loop() {
  unsigned long currentTime = millis();
  
  // Update metrics every 50ms
  if (currentTime - lastDataUpdate > 50) {
    updateMetrics();
    lastDataUpdate = currentTime;
  }
  
  // Update graph data every 500ms
  if (currentTime - lastGraphUpdate > 500) {
    updateGraphData();
    lastGraphUpdate = currentTime;
  }
  
  // Initial full draw or refresh every 5 seconds to avoid artifacts
  if (firstDraw || (currentTime - lastUpdate > 5000)) {
    gfx->fillScreen(BG_COLOR);
    
    // Draw header
    drawHeader();
    
    // Draw metric panels (2x2 grid)
    drawMetricPanel(10, 35, 110, 55, metrics[0]);  // CPU
    drawMetricPanel(125, 35, 110, 55, metrics[1]); // Memory
    drawMetricPanel(10, 95, 110, 55, metrics[2]);  // Temperature
    drawMetricPanel(125, 95, 110, 55, metrics[3]); // Voltage
    
    // Draw mini graphs
    drawMiniGraph(10, 160, 70, 30, 0);   // CPU graph
    drawMiniGraph(85, 160, 70, 30, 2);   // Temperature graph
    
    // Draw floating action button
    drawFloatingActionButton();
    
    // System info
    gfx->setTextColor(TEXT_SECONDARY);
    gfx->setTextSize(1);
    gfx->setCursor(10, 200);
    gfx->printf("Free: %dKB", ESP.getFreeHeap()/1024);
    
    gfx->setCursor(10, 215);
    gfx->printf("CPU: %dMHz", ESP.getCpuFreqMHz());
    
    firstDraw = false;
    lastUpdate = currentTime;
  }
  
  // Update header time every second (without clearing screen)
  if (currentTime - lastHeaderUpdate > 1000) {
    // Clear just the time area and redraw
    gfx->fillRect(180, 8, 50, 10, PANEL_COLOR);
    
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    seconds = seconds % 60;
    
    gfx->setTextColor(ACCENT_COLOR);
    gfx->setTextSize(1);
    gfx->setCursor(180, 10);
    gfx->printf("%02lu:%02lu", minutes, seconds);
    
    lastHeaderUpdate = currentTime;
  }
  
  // Update metric panels every 200ms (selective updates)
  static unsigned long lastMetricUpdate = 0;
  if (currentTime - lastMetricUpdate > 200) {
    // Only redraw metric panels without clearing screen
    drawMetricPanel(10, 35, 110, 55, metrics[0]);  // CPU
    drawMetricPanel(125, 35, 110, 55, metrics[1]); // Memory
    drawMetricPanel(10, 95, 110, 55, metrics[2]);  // Temperature
    drawMetricPanel(125, 95, 110, 55, metrics[3]); // Voltage
    
    lastMetricUpdate = currentTime;
  }
  
  // Serial output every 3 seconds
  static unsigned long lastSerial = 0;
  if (currentTime - lastSerial > 3000) {
    Serial.printf("System Status - CPU: %.1f%%, RAM: %.1f%%, Temp: %.1f°C, Volt: %.2fV\n",
                  metrics[0].value, metrics[1].value, metrics[2].value, metrics[3].value);
    lastSerial = currentTime;
  }
}
