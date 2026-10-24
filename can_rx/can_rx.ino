//==================================================================================//
// BMS RECEIVER - ESP32 #2
// Receives and displays battery management system parameters
//==================================================================================//
#include <SPI.h>
#include <mcp2515.h>

MCP2515 mcp2515(5);

#define BMS_DATA_ID 0x100  // CAN ID for BMS data
#define ACK_ID 0x101       // CAN ID for acknowledgment

// BMS Parameters
float voltage = 0.0;
float current = 0.0;
int charge = 0;
float temperature = 0.0;
int cycles = 0;
float power = 0.0;
String status = "Unknown";

unsigned long lastReceived = 0;
unsigned long totalReceived = 0;

//==================================================================================//
void setup() {
  Serial.begin(9600);  // Changed to 9600 to match your screenshot
  delay(2000);
  
  Serial.println("\n===========================================");
  Serial.println("   BMS RECEIVER");
  Serial.println("===========================================");
  
  SPI.begin();
  mcp2515.reset();
  delay(100);
  
  // Auto-detect crystal  
  MCP2515::ERROR result = mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  if (result != MCP2515::ERROR_OK) {
    mcp2515.setBitrate(CAN_500KBPS, MCP_16MHZ);
  }
  
  mcp2515.setNormalMode();
  
  Serial.println("✓ CAN initialized");
  Serial.println("✓ Waiting for BMS data...");
  Serial.println("===========================================\n");
  delay(1000);
}

//==================================================================================//
void loop() {
  struct can_frame rxMsg;
  
  if (mcp2515.readMessage(&rxMsg) == MCP2515::ERROR_OK) {
    if (rxMsg.can_id == BMS_DATA_ID) {
      totalReceived++;
      
      // Decode BMS data
      int voltageInt = (rxMsg.data[0] << 8) | rxMsg.data[1];
      voltage = voltageInt / 100.0;
      
      int currentInt = (rxMsg.data[2] << 8) | rxMsg.data[3];
      current = currentInt / 100.0;
      
      charge = rxMsg.data[4];
      
      temperature = rxMsg.data[5] / 10.0;
      
      cycles = (rxMsg.data[6] << 8) | rxMsg.data[7];
      
      power = voltage * current;
      
      // Determine status based on charge
      if (charge > 80) status = "Excellent";
      else if (charge > 50) status = "Good";
      else if (charge > 20) status = "Low";
      else status = "Critical";
      
      // Display battery status in the exact format you want
      displayBatteryStatus();
      
      lastReceived = millis();
    }
  }
  
  // Show "waiting" message if no data received for 5 seconds
  if (millis() - lastReceived > 5000 && totalReceived > 0) {
    Serial.println("⚠ No data received for 5 seconds...");
    lastReceived = millis();
  }
  
  delay(10);
}

//==================================================================================//
void displayBatteryStatus() {
  Serial.println("=== BATTERY STATUS ===");
  Serial.print("Voltage:    ");
  Serial.print(voltage, 2);
  Serial.println(" V");
  
  Serial.print("Current:    ");
  Serial.print(current, 2);
  Serial.println(" A");
  
  Serial.print("Charge:     ");
  Serial.print(charge);
  Serial.println(" %");
  
  Serial.print("Temp:       ");
  Serial.print(temperature, 1);
  Serial.println(" °C");
  
  Serial.print("Cycles:     ");
  Serial.println(cycles);
  
  Serial.print("Power:      ");
  Serial.print(power, 2);
  Serial.println(" W");
  
  Serial.print("Status:     ");
  Serial.println(status);
  
  Serial.println("======================\n");
}

