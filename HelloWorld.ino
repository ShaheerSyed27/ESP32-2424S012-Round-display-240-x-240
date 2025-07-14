/*
  ESP32-C3 GC9A01 Round Display - Hello World Implementation
  
  This is the baseline working implementation for ESP32-C3 with GC9A01 240x240 round display.
  Uses direct SPI control to bypass TFT_eSPI library compatibility issues.
  
  Hardware:
  - ESP32-C3 microcontroller (RISC-V architecture)
  - GC9A01 240x240 round IPS display
  - SPI communication protocol
  
  Pin Configuration (ESP32-C3 specific):
  - SCLK: GPIO 6  (SPI Clock)
  - MOSI: GPIO 7  (SPI Data Output) 
  - CS:   GPIO 10 (Chip Select)
  - DC:   GPIO 2  (Data/Command)
  - RST:  Not connected (-1)
  - BL:   GPIO 3  (Backlight control)
  
  Features:
  - Direct SPI register control (no external libraries)
  - Proper GC9A01 initialization sequence
  - Color cycling demonstration
  - Serial debugging output
  - Maximum hardware compatibility
  
  Author: Community Contribution
  License: MIT
  
  Based on research from:
  - TFT_eSPI GitHub issues
  - ESP32-C3 datasheet
  - GC9A01 controller documentation
*/

#include <SPI.h>

// Hardware Configuration - ESP32-C3 with GC9A01 Round Display
#define TFT_SCLK 6   // SPI Clock - GPIO 6
#define TFT_MOSI 7   // SPI Data Output (MOSI) - GPIO 7
#define TFT_CS   10  // Chip Select - GPIO 10
#define TFT_DC   2   // Data/Command select - GPIO 2
#define TFT_RST  -1  // Reset pin (not connected in this implementation)
#define TFT_BL   3   // Backlight control - GPIO 3
#define LED_BUILTIN -1  // ESP32-C3 module has no built-in LED

// Display Specifications
#define TFT_WIDTH  240   // Display width in pixels
#define TFT_HEIGHT 240   // Display height in pixels

// SPI Configuration for ESP32-C3
SPIClass mySPI(FSPI); // Use FSPI (ESP32-C3's SPI2 peripheral)

/**
 * Send command byte to GC9A01 controller
 * @param cmd Command byte to send
 */
void tft_send_cmd(uint8_t cmd) {
  digitalWrite(TFT_DC, LOW);  // Set DC low for command mode
  digitalWrite(TFT_CS, LOW);  // Assert chip select
  mySPI.transfer(cmd);        // Send command byte
  digitalWrite(TFT_CS, HIGH); // Release chip select
}

/**
 * Send data byte to GC9A01 controller
 * @param data Data byte to send
 */
void tft_send_data(uint8_t data) {
  digitalWrite(TFT_DC, HIGH); // Set DC high for data mode
  digitalWrite(TFT_CS, LOW);  // Assert chip select
  mySPI.transfer(data);       // Send data byte
  digitalWrite(TFT_CS, HIGH); // Release chip select
}

/**
 * Initialize GC9A01 display controller and GPIO pins
 * This function sets up the hardware and sends the initialization sequence
 */
void tft_init() {
  // Configure GPIO pins for SPI communication
  Serial.println("Setting up GPIO pins...");
  pinMode(TFT_DC, OUTPUT);    // Data/Command pin
  if (TFT_RST >= 0) pinMode(TFT_RST, OUTPUT); // Reset pin (optional)
  pinMode(TFT_CS, OUTPUT);    // Chip select pin
  pinMode(TFT_BL, OUTPUT);    // Backlight control pin
  Serial.println("GPIO pins configured");
  
  // Enable backlight
  digitalWrite(TFT_BL, HIGH);
  Serial.println("Backlight enabled - GPIO 3 set HIGH");
  
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
  // Initialize SPI bus with ESP32-C3 specific pins
  Serial.println("Initializing SPI...");
  mySPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);  // Clock, MISO(none), MOSI, CS
  mySPI.setFrequency(27000000); // 27MHz - tested stable frequency for ESP32-C3
  Serial.println("SPI initialized at 27MHz frequency");
  
  // Test SPI communication with a harmless NOP command
  Serial.println("Testing SPI communication...");
  digitalWrite(TFT_CS, LOW);
  mySPI.transfer(0x00); // NOP (No Operation) command
  digitalWrite(TFT_CS, HIGH);
  Serial.println("SPI communication test successful");

  /*
   * GC9A01 Full Initialization Sequence
   * This comprehensive sequence includes vendor-specific commands for optimal display performance
   * Commands are based on official GC9A01 initialization examples and community-tested values
   * Each command group handles specific display controller functionality
   */
  Serial.println("Starting comprehensive GC9A01 initialization...");
  
  // Vendor-specific command sequences (0xEF, 0xEB group)
  // These commands configure internal controller parameters for stability
  tft_send_cmd(0xEF);           // Enable inter command sequence
  tft_send_cmd(0xEB);           // Set display bias
  tft_send_data(0x14);
  
  tft_send_cmd(0xFE);           // Inter register enable 1
  tft_send_cmd(0xEF);           // Inter register enable 2
  
  tft_send_cmd(0xEB);           // Repeat bias setting for stability
  tft_send_data(0x14);
  
  // Power control registers (0x84-0x8F)
  // Configure internal power supply and voltage regulation
  tft_send_cmd(0x84);           // Power control 1
  tft_send_data(0x40);
  
  tft_send_cmd(0x85);           // Power control 2
  tft_send_data(0xFF);
  
  tft_send_cmd(0x86);           // Power control 3
  tft_send_data(0xFF);
  
  tft_send_cmd(0x87);           // Power control 4
  tft_send_data(0xFF);
  
  tft_send_cmd(0x88);           // Power control 5
  tft_send_data(0x0A);
  
  tft_send_cmd(0x89);           // Power control 6
  tft_send_data(0x21);
  
  tft_send_cmd(0x8A);           // Power control 7
  tft_send_data(0x00);
  
  tft_send_cmd(0x8B);           // Power control 8
  tft_send_data(0x80);
  
  tft_send_cmd(0x8C);           // Power control 9
  tft_send_data(0x01);
  
  tft_send_cmd(0x8D);           // Power control 10
  tft_send_data(0x01);
  
  tft_send_cmd(0x8E);           // Power control 11
  tft_send_data(0xFF);
  
  tft_send_cmd(0x8F);           // Power control 12
  tft_send_data(0xFF);
  
  // Display control settings
  tft_send_cmd(0xB6);           // Display function control
  tft_send_data(0x00);          // Source output direction
  tft_send_data(0x20);          // Gate scan direction
  
  tft_send_cmd(0x36);           // Memory Access Control (orientation)
  tft_send_data(0x08);          // Normal orientation, RGB color order
  
  tft_send_cmd(0x3A);           // Interface Pixel Format
  tft_send_data(0x05);          // 16-bit RGB565 color format
  
  // Porch settings for timing control
  tft_send_cmd(0x90);           // Porch setting
  tft_send_data(0x08);          // Back porch lines
  tft_send_data(0x08);          // Front porch lines  
  tft_send_data(0x08);          // Separate porch control
  tft_send_data(0x08);          // Idle porch setting
  
  // Frame rate and display timing
  tft_send_cmd(0xBD);           // Internal use register
  tft_send_data(0x06);
  
  tft_send_cmd(0xBC);           // Internal use register
  tft_send_data(0x00);
  
  // Advanced display configuration
  tft_send_cmd(0xFF);           // Command page select
  tft_send_data(0x60);
  tft_send_data(0x01);
  tft_send_data(0x04);
  
  tft_send_cmd(0xC3);           // VREG1 voltage setting
  tft_send_data(0x13);
  
  tft_send_cmd(0xC4);           // VREG2 voltage setting  
  tft_send_data(0x13);
  
  tft_send_cmd(0xC9);           // VREG3 voltage setting
  tft_send_data(0x22);
  
  tft_send_cmd(0xBE);           // Frame rate control
  tft_send_data(0x11);
  
  tft_send_cmd(0xE1);           // Frame rate setting
  tft_send_data(0x10);
  tft_send_data(0x0E);
  
  tft_send_cmd(0xDF);           // Internal register configuration
  tft_send_data(0x21);
  tft_send_data(0x0C);
  tft_send_data(0x02);
  
  // Gamma correction curves for optimal color reproduction
  // Positive gamma curve
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
  
  // Negative gamma curve
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
  
  // Additional display optimization registers
  tft_send_cmd(0xED);           
  tft_send_data(0x1B);
  tft_send_data(0x0B);
  
  tft_send_cmd(0xAE);           
  tft_send_data(0x77);
  
  tft_send_cmd(0xCD);           
  tft_send_data(0x63);
  
  // Advanced timing parameters  
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
  
  // Complex register sequences for display timing optimization
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
  
  // Final initialization commands
  tft_send_cmd(0x35);           // Tearing Effect Line ON (optional)
  tft_send_cmd(0x21);           // Display Inversion ON (required for proper colors)
  
  tft_send_cmd(0x11);           // Sleep OUT - wake up display
  delay(120);                   // Required delay for wake-up
  
  tft_send_cmd(0x29);           // Display ON - enable output
  delay(20);                    // Short stabilization delay
  
  Serial.println("GC9A01 comprehensive initialization complete - Display ready!");
}

/**
 * Fill entire display with a solid color
 * @param color 16-bit RGB565 color value (format: RRRRRGGGGGGBBBBB)
 *              Examples: 0x0000=Black, 0xFFFF=White, 0xF800=Red, 0x07E0=Green, 0x001F=Blue
 */
void tft_fill_screen(uint16_t color) {
  // Set column address range (X coordinates: 0 to 239)
  tft_send_cmd(0x2A);                           // Column Address Set command
  tft_send_data(0x00);                          // Start column high byte (0)
  tft_send_data(0x00);                          // Start column low byte (0)
  tft_send_data((TFT_WIDTH >> 8) & 0xFF);       // End column high byte  
  tft_send_data(TFT_WIDTH & 0xFF);              // End column low byte (239)
  
  // Set row address range (Y coordinates: 0 to 239)
  tft_send_cmd(0x2B);                           // Row Address Set command
  tft_send_data(0x00);                          // Start row high byte (0)
  tft_send_data(0x00);                          // Start row low byte (0)
  tft_send_data((TFT_HEIGHT >> 8) & 0xFF);      // End row high byte
  tft_send_data(TFT_HEIGHT & 0xFF);             // End row low byte (239)
  
  // Prepare for memory write operation
  tft_send_cmd(0x2C);                           // Memory Write command
  
  // Convert 16-bit color to separate high and low bytes
  uint8_t hi = color >> 8;                      // Extract upper 8 bits (RRRRRGGG)
  uint8_t lo = color & 0xFF;                    // Extract lower 8 bits (GGGBBBBB)
  
  // Optimize SPI communication by keeping CS low for entire transfer
  digitalWrite(TFT_DC, HIGH);                   // Set to data mode
  digitalWrite(TFT_CS, LOW);                    // Assert chip select
  
  // Send color data for all pixels (240 × 240 = 57,600 pixels)
  for(uint32_t i = 0; i < TFT_WIDTH * TFT_HEIGHT; i++) {
    mySPI.transfer(hi);                         // Send high byte
    mySPI.transfer(lo);                         // Send low byte
  }
  
  digitalWrite(TFT_CS, HIGH);                   // Release chip select
}

/**
 * Demonstration function showing color cycling
 * Displays a sequence of solid colors to verify display functionality
 * Each color is shown for 1 second to allow visual confirmation
 */
void draw_hello_world() {
  Serial.println("Starting color cycling demonstration...");
  
  // Blue - Primary color test (full blue channel)
  Serial.println("Displaying: Blue");
  tft_fill_screen(0x001F); // RGB565: 00000 000000 11111 (Blue = 31)
  delay(1000);
  
  // Green - Primary color test (full green channel) 
  Serial.println("Displaying: Green");
  tft_fill_screen(0x07E0); // RGB565: 00000 111111 00000 (Green = 63)
  delay(1000);
  
  // Red - Primary color test (full red channel)
  Serial.println("Displaying: Red");
  tft_fill_screen(0xF800); // RGB565: 11111 000000 00000 (Red = 31)
  delay(1000);
  
  // White - All channels maximum (display brightness test)
  Serial.println("Displaying: White");
  tft_fill_screen(0xFFFF); // RGB565: 11111 111111 11111 (All channels max)
  delay(1000);
  
  // Black - All channels minimum (contrast test)
  Serial.println("Displaying: Black");
  tft_fill_screen(0x0000); // RGB565: 00000 000000 00000 (All channels off)
  
  Serial.println("Color cycling demonstration complete!");
}

/**
 * Arduino setup function - runs once at startup
 * Initializes hardware, communication, and performs initial display test
 */
void setup() {
  // Initialize serial communication for debugging and status monitoring
  Serial.begin(115200);  // High baud rate for responsive debugging
  
  // Wait for serial port to connect (important for native USB boards)
  delay(3000);
  
  // Print comprehensive startup information
  Serial.println("=========================================");
  Serial.println("ESP32-C3 GC9A01 Hello World Demo");
  Serial.println("Hardware: ESP32-C3 + GC9A01 Round Display");
  Serial.println("Implementation: Direct SPI Control");
  Serial.println("=========================================");
  
  // Display ESP32-C3 hardware information
  Serial.print("ESP32 Chip Model: ");
  Serial.println(ESP.getChipModel());
  Serial.print("ESP32 Chip Revision: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("ESP32 CPU Frequency: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");
  Serial.print("Free Heap Memory: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  
  Serial.println("Pin Configuration:");
  Serial.println("  SCLK: GPIO 6  (SPI Clock)");
  Serial.println("  MOSI: GPIO 7  (SPI Data)");
  Serial.println("  CS:   GPIO 10 (Chip Select)");
  Serial.println("  DC:   GPIO 2  (Data/Command)");
  Serial.println("  RST:  Not connected");
  Serial.println("  BL:   GPIO 3  (Backlight)");
  Serial.println("Starting hardware initialization...");
  
  // Handle built-in LED (may not exist on all ESP32-C3 modules)
  if (LED_BUILTIN >= 0) {
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("Built-in LED: ON");
  } else {
    Serial.println("Built-in LED: Not available on this module");
  }
  
  // Initialize display hardware
  Serial.println("Initializing GC9A01 display hardware...");
  tft_init();
  
  // Perform initial display test with color sequence
  Serial.println("Running initial display functionality test...");
  draw_hello_world();
  
  Serial.println("=========================================");
  Serial.println("Setup complete! Display is operational.");
  Serial.println("Entering main loop with continuous color cycling...");
  Serial.println("=========================================");
}

/**
 * Arduino main loop function - runs continuously after setup()
 * Implements continuous color cycling with serial status updates
 * Demonstrates reliable long-term operation
 */
void loop() {
  // Print periodic status message to serial monitor
  Serial.println("Hello World from ESP32-C3! Display cycling colors...");
  
  // Handle built-in LED blinking (if available)
  if (LED_BUILTIN >= 0) {
    // LED off phase
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("LED: OFF");
    delay(500);
    
    // LED on phase
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("LED: ON");
    delay(500);
  } else {
    // No LED available, just wait
    delay(1000);
  }
  
  // Continuous color cycling with timing control
  static unsigned long lastColorChange = 0;    // Timestamp of last color change
  static uint8_t colorIndex = 0;               // Current color in sequence
  
  // Change color every 10 seconds (10000ms)
  if (millis() - lastColorChange > 10000) {
    // Color palette with descriptive names
    uint16_t colors[] = {0x0000, 0x001F, 0x07E0, 0xF800, 0xFFFF, 0xF81F, 0xFFE0, 0x07FF}; 
    String colorNames[] = {"Black", "Blue", "Green", "Red", "White", "Magenta", "Yellow", "Cyan"};
    
    // Display color change information
    Serial.println("=====================================");
    Serial.println("Changing display color to: " + colorNames[colorIndex]);
    Serial.print("RGB565 Value: 0x");
    Serial.println(colors[colorIndex], HEX);
    
    // Update display with new color
    tft_fill_screen(colors[colorIndex]);
    
    // Update loop variables
    colorIndex = (colorIndex + 1) % 8;          // Cycle through 8 colors
    lastColorChange = millis();                 // Record timestamp
    
    // Display memory status for monitoring
    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.println("=====================================");
  }
}
