#include <Wire.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// --- HARDWARE SETUP ---
const int MPU_addr = 0x68; 
RF24 radio(9, 10);         
const byte address[6] = "00001"; // Must match receiver perfectly

// --- DATA STRUCTURE ---
// Matches the receiver's struct exactly
struct DataPacket {
  int x_tilt; // Steering
  int y_tilt; // Throttle
};
DataPacket myData;

void setup() {
  Serial.begin(9600);

  // Initialize the MPU6050
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // Power management register
  Wire.write(0);     // Wake up
  Wire.endTransmission(true);

  // Initialize the nRF24L01 Radio
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);  
  radio.stopListening(); // Transmitter mode
}

void loop() {
  // --- 1. Request Data from MPU6050 ---
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 6, true); 
  
  // Read the raw accelerometer data
  int16_t AcX = Wire.read() << 8 | Wire.read();  
  int16_t AcY = Wire.read() << 8 | Wire.read();  
  int16_t AcZ = Wire.read() << 8 | Wire.read();  

  // --- 2. Calculate Pitch (Y) and Roll (X) ---
  // Y-axis: Forward and Backward tilt
  int calculated_y_angle = atan2(-AcX, sqrt(pow(AcY, 2) + pow(AcZ, 2))) * 180 / PI;
  
  // X-axis: Left and Right tilt
  int calculated_x_angle = atan2(AcY, sqrt(pow(AcX, 2) + pow(AcZ, 2))) * 180 / PI;

  // --- 3. Package the Data ---
  myData.y_tilt = constrain(calculated_y_angle, -90, 90);
  myData.x_tilt = constrain(calculated_x_angle, -90, 90);

  // --- 4. Transmit ---
  radio.write(&myData, sizeof(DataPacket));

  // Debugging output
  Serial.print("X: "); Serial.print(myData.x_tilt);
  Serial.print(" | Y: "); Serial.println(myData.y_tilt);

  delay(20); 
}