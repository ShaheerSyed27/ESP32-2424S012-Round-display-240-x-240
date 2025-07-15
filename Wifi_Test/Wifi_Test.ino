/*
  ESP32-C3 WiFi Scanner with GC9A01 Display
  
  This program demonstrates WiFi scanning capabilities on ESP32-C3 with visual feedback
  on the GC9A01 round display. Shows scanning progress, found networks, and signal strengths.
  
  Hardware:
  - ESP32-C3 microcontroller with WiFi capability
  - GC9A01 240x240 round IPS display
  - Arduino_GFX library for optimized display rendering
  
  Features:
  - Real-time scanning status display
  - WiFi network list with signal strength indicators
  - Network security type identification
  - Scrollable network list for many results
  - Signal strength visualization with bars
  - Auto-refresh scanning every 10 seconds
  
  Pin Configuration:
  - SCLK: GPIO 6  (SPI Clock)
  - MOSI: GPIO 7  (SPI Data Output)
  - CS:   GPIO 10 (Chip Select)
  - DC:   GPIO 2  (Data/Command)
  - RST:  Not connected
  - BL:   GPIO 3  (Backlight control)
  
  Author: Community Contribution
  License: MIT
*/

#include <WiFi.h>
#include <Arduino_GFX_Library.h>

// Hardware Configuration - Arduino_GFX with ESP32-C3 SPI
Arduino_DataBus *bus = new Arduino_ESP32SPI(
  2,                     // DC (Data/Command) pin - GPIO 2
  10,                    // CS (Chip Select) pin - GPIO 10
  6,                     // SCK (Clock) pin - GPIO 6
  7,                     // MOSI (Data) pin - GPIO 7
  GFX_NOT_DEFINED        // MISO not used for display
);

// GC9A01 Display Controller
Arduino_GFX *gfx = new Arduino_GC9A01(
  bus,                   // SPI bus interface
  GFX_NOT_DEFINED,       // RST pin (not connected)
  0,                     // Rotation (0 = normal)
  true                   // IPS panel (better color accuracy)
);

// Color Palette for WiFi Scanner UI
#define BG_COLOR        0x0020  // Dark blue background
#define HEADER_COLOR    0x1082  // Header panel color
#define ACCENT_COLOR    0x051D  // Bright blue accent
#define SUCCESS_COLOR   0x07E0  // Green for strong signal
#define WARNING_COLOR   0xFFE0  // Yellow for medium signal
#define DANGER_COLOR    0xF800  // Red for weak signal
#define TEXT_PRIMARY    0xFFFF  // White primary text
#define TEXT_SECONDARY  0x8410  // Gray secondary text
#define PANEL_COLOR     0x2104  // Panel background
#define BORDER_COLOR    0x4208  // Border color

// WiFi Network Structure
struct WiFiNetwork {
  String ssid;
  int32_t rssi;
  wifi_auth_mode_t encryptionType;
  bool isHidden;
};

// Global Variables
WiFiNetwork networks[20];        // Store up to 20 networks
int networkCount = 0;           // Number of networks found
int scrollOffset = 0;           // For scrolling through networks
bool isScanning = false;        // Scanning state
unsigned long lastScan = 0;     // Last scan timestamp
unsigned long scanStartTime = 0; // Scan start time for animation

/**
 * Arduino setup function - Initialize hardware and start WiFi scanning
 */
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("========================================");
  Serial.println("ESP32-C3 WiFi Scanner Starting...");
  Serial.println("Hardware: ESP32-C3 + GC9A01 Display");
  Serial.println("========================================");
  
  // Initialize display backlight
  pinMode(3, OUTPUT);
  digitalWrite(3, HIGH);
  Serial.println("Display backlight: ON");
  
  // Initialize display
  Serial.println("Initializing display...");
  gfx->begin();
  gfx->fillScreen(BG_COLOR);
  
  // Draw initial UI
  drawHeader();
  drawScanningStatus();
  
  // Initialize WiFi in station mode
  Serial.println("Initializing WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.println("========================================");
  Serial.println("WiFi Scanner Ready!");
  Serial.println("Starting network scan...");
  Serial.println("========================================");
  
  // Start initial scan
  startWiFiScan();
}

/**
 * Draw the header section with title and status
 */
void drawHeader() {
  // Header background
  gfx->fillRect(0, 0, 240, 30, HEADER_COLOR);
  gfx->drawFastHLine(0, 30, 240, ACCENT_COLOR);
  
  // Title
  gfx->setTextColor(TEXT_PRIMARY);
  gfx->setTextSize(1);
  gfx->setCursor(10, 10);
  gfx->println("ESP32-C3 WiFi Scanner");
  
  // WiFi icon (simple representation)
  drawWiFiIcon(200, 15, TEXT_PRIMARY);
}

/**
 * Draw WiFi icon at specified position
 */
void drawWiFiIcon(int x, int y, uint16_t color) {
  // Simple WiFi icon using arcs
  gfx->drawCircle(x, y, 3, color);
  gfx->drawCircle(x, y, 7, color);
  gfx->drawCircle(x, y, 11, color);
  gfx->fillCircle(x, y, 2, color);
}

/**
 * Draw scanning status with animated indicator
 */
void drawScanningStatus() {
  // Clear status area
  gfx->fillRect(0, 35, 240, 25, BG_COLOR);
  
  if (isScanning) {
    // Animated scanning indicator
    gfx->setTextColor(ACCENT_COLOR);
    gfx->setTextSize(1);
    gfx->setCursor(10, 45);
    gfx->print("Scanning for networks");
    
    // Animated dots
    unsigned long elapsed = millis() - scanStartTime;
    int dots = (elapsed / 500) % 4;
    for (int i = 0; i < dots; i++) {
      gfx->print(".");
    }
    
    // Progress bar
    int progress = (elapsed % 3000) * 200 / 3000;
    gfx->fillRect(20, 52, progress, 3, ACCENT_COLOR);
    gfx->drawRect(20, 52, 200, 3, BORDER_COLOR);
    
  } else {
    // Show scan results summary
    gfx->setTextColor(TEXT_PRIMARY);
    gfx->setTextSize(1);
    gfx->setCursor(10, 45);
    gfx->print("Found ");
    gfx->print(networkCount);
    gfx->print(" networks");
    
    // Last scan time
    gfx->setTextColor(TEXT_SECONDARY);
    gfx->setCursor(130, 45);
    gfx->print("Updated: ");
    gfx->print((millis() - lastScan) / 1000);
    gfx->print("s ago");
  }
}

/**
 * Start WiFi network scanning
 */
void startWiFiScan() {
  isScanning = true;
  scanStartTime = millis();
  Serial.println("Starting WiFi scan...");
  
  // Start asynchronous scan
  WiFi.scanNetworks(true, false, false, 300);
}

/**
 * Process completed WiFi scan results
 */
void processScanResults() {
  int n = WiFi.scanComplete();
  
  if (n == WIFI_SCAN_FAILED) {
    Serial.println("WiFi scan failed!");
    isScanning = false;
    return;
  }
  
  networkCount = min(n, 20);  // Limit to 20 networks
  Serial.print("Networks found: ");
  Serial.println(networkCount);
  
  // Store network information
  for (int i = 0; i < networkCount; i++) {
    networks[i].ssid = WiFi.SSID(i);
    networks[i].rssi = WiFi.RSSI(i);
    networks[i].encryptionType = WiFi.encryptionType(i);
    networks[i].isHidden = (networks[i].ssid.length() == 0);
    
    // Debug output
    Serial.printf("%d: %s (%d dBm) %s\n", 
                  i + 1,
                  networks[i].isHidden ? "[Hidden]" : networks[i].ssid.c_str(),
                  networks[i].rssi,
                  getEncryptionTypeString(networks[i].encryptionType).c_str());
  }
  
  // Sort networks by signal strength (strongest first)
  sortNetworksBySignal();
  
  isScanning = false;
  lastScan = millis();
  scrollOffset = 0;  // Reset scroll position
  
  // Clear scan results from memory
  WiFi.scanDelete();
  
  Serial.println("Scan completed and processed");
}

/**
 * Sort networks by signal strength (strongest first)
 */
void sortNetworksBySignal() {
  for (int i = 0; i < networkCount - 1; i++) {
    for (int j = i + 1; j < networkCount; j++) {
      if (networks[i].rssi < networks[j].rssi) {
        // Swap networks
        WiFiNetwork temp = networks[i];
        networks[i] = networks[j];
        networks[j] = temp;
      }
    }
  }
}

/**
 * Get encryption type as readable string
 */
String getEncryptionTypeString(wifi_auth_mode_t encryptionType) {
  switch (encryptionType) {
    case WIFI_AUTH_OPEN: return "Open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-E";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    default: return "Unknown";
  }
}

/**
 * Get color based on signal strength
 */
uint16_t getSignalColor(int32_t rssi) {
  if (rssi >= -50) return SUCCESS_COLOR;      // Excellent signal
  else if (rssi >= -70) return WARNING_COLOR; // Good signal
  else return DANGER_COLOR;                   // Poor signal
}

/**
 * Draw signal strength bars
 */
void drawSignalBars(int x, int y, int32_t rssi) {
  uint16_t color = getSignalColor(rssi);
  
  // Convert RSSI to number of bars (1-4)
  int bars = 1;
  if (rssi >= -50) bars = 4;      // Excellent
  else if (rssi >= -60) bars = 3; // Good
  else if (rssi >= -70) bars = 2; // Fair
  else bars = 1;                  // Poor
  
  // Draw signal bars
  for (int i = 0; i < 4; i++) {
    int barHeight = 2 + (i * 2);
    uint16_t barColor = (i < bars) ? color : BORDER_COLOR;
    gfx->fillRect(x + (i * 3), y - barHeight, 2, barHeight, barColor);
  }
}

/**
 * Draw the network list
 */
void drawNetworkList() {
  // Clear network list area
  gfx->fillRect(0, 65, 240, 175, BG_COLOR);
  
  if (networkCount == 0) {
    gfx->setTextColor(TEXT_SECONDARY);
    gfx->setTextSize(1);
    gfx->setCursor(20, 120);
    gfx->println("No networks found");
    return;
  }
  
  // Draw network entries
  int maxVisible = 8;  // Maximum visible networks
  int startIndex = scrollOffset;
  int endIndex = min(startIndex + maxVisible, networkCount);
  
  for (int i = startIndex; i < endIndex; i++) {
    int yPos = 70 + ((i - startIndex) * 20);
    drawNetworkEntry(i, yPos);
  }
  
  // Draw scroll indicator if needed
  if (networkCount > maxVisible) {
    drawScrollIndicator();
  }
}

/**
 * Draw individual network entry
 */
void drawNetworkEntry(int index, int yPos) {
  WiFiNetwork &network = networks[index];
  
  // Network panel background
  gfx->fillRect(5, yPos, 230, 18, PANEL_COLOR);
  gfx->drawRect(5, yPos, 230, 18, BORDER_COLOR);
  
  // Network name
  gfx->setTextColor(TEXT_PRIMARY);
  gfx->setTextSize(1);
  gfx->setCursor(10, yPos + 5);
  
  if (network.isHidden) {
    gfx->setTextColor(TEXT_SECONDARY);
    gfx->print("[Hidden Network]");
  } else {
    // Truncate long SSIDs
    String displayName = network.ssid;
    if (displayName.length() > 20) {
      displayName = displayName.substring(0, 17) + "...";
    }
    gfx->print(displayName);
  }
  
  // Signal strength bars
  drawSignalBars(200, yPos + 15, network.rssi);
  
  // Signal strength in dBm
  gfx->setTextColor(TEXT_SECONDARY);
  gfx->setTextSize(1);
  gfx->setCursor(140, yPos + 5);
  gfx->print(network.rssi);
  gfx->print("dBm");
  
  // Security indicator
  if (network.encryptionType != WIFI_AUTH_OPEN) {
    gfx->fillRect(215, yPos + 2, 3, 3, WARNING_COLOR);  // Lock icon placeholder
  }
}

/**
 * Draw scroll indicator
 */
void drawScrollIndicator() {
  // Scroll bar background
  gfx->fillRect(235, 65, 4, 175, BORDER_COLOR);
  
  // Scroll position indicator
  int maxScroll = max(0, networkCount - 8);
  if (maxScroll > 0) {
    int indicatorHeight = 175 * 8 / networkCount;
    int indicatorPos = 65 + (scrollOffset * (175 - indicatorHeight) / maxScroll);
    gfx->fillRect(235, indicatorPos, 4, indicatorHeight, ACCENT_COLOR);
  }
}

/**
 * Main loop - Handle scanning and display updates
 */
void loop() {
  // Check if scan is complete
  if (isScanning) {
    int scanResult = WiFi.scanComplete();
    if (scanResult >= 0) {
      processScanResults();
    }
    
    // Update scanning animation
    drawScanningStatus();
    delay(200);
  } else {
    // Update display
    drawScanningStatus();
    drawNetworkList();
    
    // Auto-refresh scan every 10 seconds
    if (millis() - lastScan > 10000) {
      startWiFiScan();
    }
    
    delay(1000);
  }
  
  // Optional: Add button handling for scrolling
  // This could be expanded to handle physical buttons for navigation
}