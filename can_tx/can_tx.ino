//==================================================================================//
// BMS TRANSMITTER - ESP32 #1
// Sends dummy battery management system parameters
//==================================================================================//
#include <SPI.h>
#include <mcp2515.h>

MCP2515 mcp2515(5);

#define BMS_DATA_ID 0x100  // CAN ID for BMS data
#define ACK_ID 0x101       // CAN ID for acknowledgment

// BMS Parameters (dummy values that will change)
float voltage = 12.50;      // Voltage in V
float current = 2.50;       // Current in A
int charge = 85;            // Charge percentage
float temperature = 25.0;   // Temperature in °C
int cycles = 240;           // Charge cycles
float power = 31.25;        // Power in W
String status = "Excellent"; // Battery status

unsigned long lastSend = 0;
unsigned long messageCount = 0;

//==================================================================================//
void setup() {
  Serial.begin(9600);  // Changed to 9600 to match your screenshot
  delay(2000);
  
  Serial.println("\n===========================================");
  Serial.println("   BMS TRANSMITTER");
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
  Serial.println("✓ Sending BMS data...");
  Serial.println("===========================================\n");
  delay(1000);
}

//==================================================================================//
void loop() {
  if (millis() - lastSend >= 2000) {
    updateBMSData();
    sendBMSData();
    lastSend = millis();
  }
}

//==================================================================================//
void updateBMSData() {
  // Simulate changing battery parameters
  voltage += random(-10, 10) / 100.0;
  if (voltage < 11.0) voltage = 11.0;
  if (voltage > 13.5) voltage = 13.5;
  
  current += random(-20, 20) / 100.0;
  if (current < 0.5) current = 0.5;
  if (current > 5.0) current = 5.0;
  
  charge += random(-2, 2);
  if (charge < 0) charge = 0;
  if (charge > 100) charge = 100;
  
  temperature += random(-5, 5) / 10.0;
  if (temperature < 20.0) temperature = 20.0;
  if (temperature > 35.0) temperature = 35.0;
  
  cycles += random(0, 2);
  if (cycles > 500) cycles = 240;
  
  power = voltage * current;
  
  // Update status based on charge
  if (charge > 80) status = "Excellent";
  else if (charge > 50) status = "Good";
  else if (charge > 20) status = "Low";
  else status = "Critical";
}

//==================================================================================//
void sendBMSData() {
  messageCount++;
  
  // Pack BMS data into CAN frames
  // We'll send 4 separate CAN messages to fit all data
  
  // Frame 1: Voltage, Current (8 bytes)
  struct can_frame frame1;
  frame1.can_id = BMS_DATA_ID;
  frame1.can_dlc = 8;
  
  int voltageInt = (int)(voltage * 100);    // 12.70V -> 1270
  int currentInt = (int)(current * 100);    // 2.50A -> 250
  
  frame1.data[0] = (voltageInt >> 8) & 0xFF;
  frame1.data[1] = voltageInt & 0xFF;
  frame1.data[2] = (currentInt >> 8) & 0xFF;
  frame1.data[3] = currentInt & 0xFF;
  frame1.data[4] = charge;
  frame1.data[5] = (int)(temperature * 10) & 0xFF;
  frame1.data[6] = (cycles >> 8) & 0xFF;
  frame1.data[7] = cycles & 0xFF;
  
  Serial.print("Transmitting: BAT:");
  Serial.print(voltage, 2);
  Serial.print(",");
  Serial.print(current, 2);
  Serial.print(",");
  Serial.print(charge);
  Serial.print(",");
  Serial.print(temperature, 1);
  Serial.print(",");
  Serial.print(cycles);
  
  if (mcp2515.sendMessage(&frame1) == MCP2515::ERROR_OK) {
    Serial.println(" → SENT ✓");
  } else {
    Serial.println(" → FAILED ✗");
  }
  
  delay(50);
}
