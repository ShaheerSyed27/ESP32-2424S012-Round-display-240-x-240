/*
  ESP32-C3 Smooth Display Dashboard - Anti-Flickering Implementation
  
  This advanced implementation demonstrates modern UI design principles with zero-flickering updates.
  Uses selective drawing techniques to update only changed elements, providing smooth visual experience.
  
  Hardware:
  - ESP32-C3 microcontroller (RISC-V architecture)
  - GC9A01 240x240 round IPS display
  - Arduino_GFX library for optimized graphics rendering
  
  Key Features:
  - Selective update system (only redraws changed elements)
  - Change tracking for all metrics
  - Modern dashboard UI with panels and indicators
  - Real-time clock display
  - Memory-efficient rendering
  - Zero-flicker operation
  
  Anti-Flickering Techniques:
  1. Static elements drawn once during initialization
  2. Dynamic elements updated only when values change
  3. Minimal drawing operations with precise positioning
  4. Background preservation during updates
  
  Pin Configuration (same as HelloWorld):
  - SCLK: GPIO 6  (SPI Clock)
  - MOSI: GPIO 7  (SPI Data Output)
  - CS:   GPIO 10 (Chip Select)
  - DC:   GPIO 2  (Data/Command)
  - RST:  Not connected
  - BL:   GPIO 3  (Backlight control)
  
  Author: Community Contribution
  License: MIT
  Based on: Arduino_GFX anti-flickering techniques
*/

#include <Arduino_GFX_Library.h>

// Hardware Configuration - Arduino_GFX SPI Setup
// Creates SPI bus interface with ESP32-C3 specific pins
Arduino_DataBus *bus = new Arduino_ESP32SPI(
  2,                     // DC (Data/Command) pin - GPIO 2
  10,                    // CS (Chip Select) pin - GPIO 10  
  6,                     // SCK (Clock) pin - GPIO 6
  7,                     // MOSI (Data) pin - GPIO 7
  GFX_NOT_DEFINED        // MISO not used for display
);

// GC9A01 Display Controller Setup
Arduino_GFX *gfx = new Arduino_GC9A01(
  bus,                   // SPI bus interface
  GFX_NOT_DEFINED,       // RST pin (not connected)
  0,                     // Rotation (0 = normal orientation)
  true                   // IPS panel (better color accuracy)
);

// Modern UI Color Palette (16-bit RGB565 format)
#define BG_COLOR        0x0841  // Dark blue-gray background
#define PANEL_COLOR     0x1082  // Lighter panel background
#define ACCENT_COLOR    0x051D  // Bright blue accent
#define SUCCESS_COLOR   0x07E0  // Green for normal status
#define WARNING_COLOR   0xFFE0  // Yellow for warnings
#define DANGER_COLOR    0xF800  // Red for alerts
#define TEXT_PRIMARY    0xFFFF  // White primary text
#define TEXT_SECONDARY  0x8410  // Gray secondary text

/**
 * Metric data structure for change tracking and selective updates
 * This structure enables the anti-flickering system by tracking what needs to be redrawn
 */
struct SimpleMetric {
  float value;               // Current metric value
  float lastDisplayedValue;  // Last value actually drawn on screen
  const char* label;         // Metric name (e.g., "CPU")
  const char* unit;          // Unit of measurement (e.g., "%")
  uint16_t color;           // Display color for this metric
  int x, y, w, h;           // Position and size on screen
  bool needsUpdate;         // Flag indicating if redraw is needed
};

// Dashboard Metrics Configuration
// Each metric has dedicated screen area and tracking for smooth updates
SimpleMetric metrics[] = {
  {0, -999, "CPU", "%", ACCENT_COLOR, 20, 50, 90, 60, true},      // Top-left panel
  {0, -999, "MEM", "%", SUCCESS_COLOR, 130, 50, 90, 60, true},    // Top-right panel
  {0, -999, "TEMP", "C", WARNING_COLOR, 20, 130, 90, 60, true},   // Bottom-left panel
  {0, -999, "VOLT", "V", DANGER_COLOR, 130, 130, 90, 60, true}    // Bottom-right panel
};

// Global State Variables
const int numMetrics = 4;
unsigned long lastUpdate = 0;        // Timestamp for metric updates
unsigned long lastTimeUpdate = 0;    // Timestamp for clock updates
bool displayInitialized = false;     // Tracks initialization state

/**
 * Arduino setup function - initializes hardware and draws static UI elements
 * Anti-flickering strategy: Draw all static elements once, then only update dynamic content
 */
void setup() {
  Serial.begin(115200);
  delay(1000); // Allow serial connection to stabilize
  
  Serial.println("========================================");
  Serial.println("ESP32-C3 Smooth Dashboard Starting...");
  Serial.println("Anti-Flickering Implementation");
  Serial.println("========================================");
  
  // Enable display backlight (GPIO 3)
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  Serial.println("Backlight: ON");
  
  // Initialize Arduino_GFX display controller
  Serial.println("Initializing Arduino_GFX display...");
  gfx->begin();
  Serial.println("Display controller ready");
  
  // Fill screen with background color (done once)
  gfx->fillScreen(BG_COLOR);
  Serial.println("Background color applied");
  
  // Draw all static UI elements (done once to prevent flickering)
  Serial.println("Drawing static UI elements...");
  initializeDisplay();
  displayInitialized = true;
  
  Serial.println("========================================");
  Serial.println("Smooth dashboard initialization complete!");
  Serial.println("Starting selective update loop...");
  Serial.println("========================================");
}

/**
 * Initialize static display elements (called once to prevent flickering)
 * This function draws all non-changing UI elements that serve as the dashboard framework
 */
void initializeDisplay() {
  Serial.println("Drawing header bar...");
  // Header bar background (drawn once)
  gfx->fillRect(0, 0, 240, 35, PANEL_COLOR);
  gfx->drawFastHLine(0, 35, 240, ACCENT_COLOR); // Accent line below header
  
  // Dashboard title (static text)
  gfx->setTextColor(TEXT_PRIMARY);
  gfx->setTextSize(1);
  gfx->setCursor(10, 15);
  gfx->println("ESP32-C3 SMOOTH DASHBOARD");
  
  // Connection status indicator (green circle)
  gfx->fillCircle(210, 18, 5, SUCCESS_COLOR);
  
  Serial.println("Drawing metric panel backgrounds...");
  // Draw metric panels background and labels (static elements)
  for (int i = 0; i < numMetrics; i++) {
    // Panel background with rounded corners
    gfx->fillRoundRect(metrics[i].x, metrics[i].y, metrics[i].w, metrics[i].h, 6, PANEL_COLOR);
    
    // Colored border for each metric type
    gfx->drawRoundRect(metrics[i].x, metrics[i].y, metrics[i].w, metrics[i].h, 6, metrics[i].color);
    
    // Metric label (drawn once - never changes)
    gfx->setTextColor(TEXT_SECONDARY);
    gfx->setTextSize(1);
    gfx->setCursor(metrics[i].x + 5, metrics[i].y + 5);
    gfx->println(metrics[i].label);
    
    Serial.print("Panel ");
    Serial.print(i);
    Serial.print(" (");
    Serial.print(metrics[i].label);
    Serial.println(") background drawn");
  }
  
  Serial.println("Static UI elements complete - ready for selective updates");
}
    
    
    // Unit labels (static - drawn once)
    gfx->setCursor(metrics[i].x + 5, metrics[i].y + 45);
    gfx->println(metrics[i].unit);
  }
  
  // Footer information (static)
  gfx->setTextColor(TEXT_SECONDARY);
  gfx->setTextSize(1);
  gfx->setCursor(20, 210);
  gfx->println("No Flicker Mode");
  
  Serial.println("Static UI elements complete - ready for selective updates");
}

/**
 * Update metric values with simulated sensor data
 * This function generates realistic changing values and determines what needs redrawing
 */
void updateMetrics() {
  // Generate simulated sensor data with realistic variations
  metrics[0].value = 25 + (millis() / 200) % 50;            // CPU: 25-75%
  metrics[1].value = 35 + (millis() / 300) % 35;            // Memory: 35-70%
  metrics[2].value = 22 + (analogRead(A0) / 4095.0) * 20;   // Temperature: 22-42°C
  metrics[3].value = 3.1 + (analogRead(A1) / 4095.0) * 0.4; // Voltage: 3.1-3.5V
  
  // Selective update logic - only mark for redraw if change is significant
  for (int i = 0; i < numMetrics; i++) {
    float diff = abs(metrics[i].value - metrics[i].lastDisplayedValue);
    if (diff > 0.5) {  // Threshold to prevent unnecessary redraws
      metrics[i].needsUpdate = true;
    }
  }
}

/**
 * Selective metric display update (anti-flickering core function)
 * Only redraws the specific value area for metrics that have changed
 * @param index Index of the metric to update (0-3)
 */
void updateMetricDisplay(int index) {
  SimpleMetric &metric = metrics[index];
  
  // Skip update if no change detected
  if (!metric.needsUpdate) return;
  
  // Anti-flicker technique: Clear only the value area, not the entire panel
  gfx->fillRect(metric.x + 3, metric.y + 18, metric.w - 6, 25, PANEL_COLOR);
  
  // Draw the updated value with appropriate formatting
  gfx->setTextColor(TEXT_PRIMARY);
  gfx->setTextSize(2);
  gfx->setCursor(metric.x + 8, metric.y + 22);
  
  // Format value based on metric type
  char valueStr[10];
  if (index == 3) {  // Voltage requires 2 decimal places
    sprintf(valueStr, "%.2f", metric.value);
  } else {
    sprintf(valueStr, "%.1f", metric.value);
  }
  gfx->println(valueStr);
  
  // Update progress bar with selective clearing
  int barX = metric.x + 5;
  int barY = metric.y + metric.h - 8;
  int barW = metric.w - 10;
  
  // Clear only the progress bar area
  gfx->fillRect(barX, barY, barW, 4, PANEL_COLOR);
  
  // Draw new progress bar proportional to value
  float maxVal = (index < 2) ? 100.0 : (index == 2) ? 60.0 : 5.0; // Different scales per metric
  int progress = (metric.value / maxVal) * barW;
  gfx->fillRect(barX, barY, progress, 4, metric.color);
  
  // Update tracking variables
  metric.lastDisplayedValue = metric.value;
  metric.needsUpdate = false;
}

/**
 * Selective time display update in header
 * Updates the time display only when the time actually changes
 */
void updateTimeDisplay() {
  static char lastTimeStr[10] = "";  // Track last displayed time
  
  // Calculate current time from millis()
  unsigned long seconds = millis() / 1000;
  unsigned long minutes = seconds / 60;
  seconds = seconds % 60;
  
  // Format current time string
  char currentTimeStr[10];
  sprintf(currentTimeStr, "%02lu:%02lu", minutes, seconds);
  
  // Anti-flicker: Only update display if time string changed
  if (strcmp(currentTimeStr, lastTimeStr) != 0) {
    // Clear only the time display area
    gfx->fillRect(150, 12, 40, 12, PANEL_COLOR);
    
    // Draw updated time
    gfx->setTextColor(ACCENT_COLOR);
    gfx->setTextSize(1);
    gfx->setCursor(155, 15);
    gfx->println(currentTimeStr);
    
    // Update tracking variable
    strcpy(lastTimeStr, currentTimeStr);
  }
}

/**
 * Arduino main loop - implements the selective update system
 * This is the core of the anti-flickering implementation
 */
void loop() {
  unsigned long currentTime = millis();
  
  // Safety check - don't start updates until display is fully initialized
  if (!displayInitialized) return;
  
  // Update metric values and check for changes every 100ms
  if (currentTime - lastUpdate > 100) {
    updateMetrics();  // Generate new values and mark changed metrics
    
    // Selective update: Only redraw metrics that have changed
    for (int i = 0; i < numMetrics; i++) {
      updateMetricDisplay(i);  // Each function checks its own needsUpdate flag
    }
    
    lastUpdate = currentTime;
  }
  
  // Update time display every second (selective update)
  if (currentTime - lastTimeUpdate > 1000) {
    updateTimeDisplay();  // Only redraws if time string changed
    lastTimeUpdate = currentTime;
  }
  
  // Debug output to serial monitor every 3 seconds
  static unsigned long lastSerial = 0;
  if (currentTime - lastSerial > 3000) {
    Serial.printf("Smooth Update - CPU: %.1f%%, MEM: %.1f%%, TEMP: %.1f°C, VOLT: %.2fV\n",
                  metrics[0].value, metrics[1].value, metrics[2].value, metrics[3].value);
    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    lastSerial = currentTime;
  }
}
