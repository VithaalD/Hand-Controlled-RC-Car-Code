#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// --- PIN DEFINITIONS ---
// Left Motor (L298N)
#define IN1 2
#define IN2 3
#define ENA 5 // PWM pin for Left Speed

// Right Motor (L298N)
#define IN3 4
#define IN4 7
#define ENB 6 // PWM pin for Right Speed

// Radio
#define RADIO_CE 9
#define RADIO_CSN 10

// --- RADIO SETUP ---
RF24 radio(RADIO_CE, RADIO_CSN);
const byte address[6] = "00001"; // Must match Transmitter exactly

// --- DATA STRUCTURE ---
struct DataPacket {
  int x_tilt; // Steering
  int y_tilt; // Throttle
};
DataPacket myData;

// --- FAILSAFE TIMER ---
unsigned long lastReceiveTime = 0;

void setup() {
  Serial.begin(9600);

  // Configure L298N motor pins as outputs
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Start with motors completely off
  stopMotors();

  // Initialize the nRF24L01 Radio
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN); // Keep at MIN while testing close together
  radio.startListening();        // Set module to Receiver mode
}

void loop() {
  // --- 1. Catch Incoming Radio Data ---
  if (radio.available()) {
    radio.read(&myData, sizeof(DataPacket));
    lastReceiveTime = millis(); // Reset the failsafe timer
    
    executeMovement(); // Process the angles into motor movements
  }

  // --- 2. Signal Failsafe ---
  // If 500 milliseconds pass without a packet, stop the motors
  if (millis() - lastReceiveTime > 500) {
    stopMotors();
  }
}

void executeMovement() {
  // Deadband: Ignore tilt under 15 degrees to account for hand shaking
  int deadband = 15; 
  
  int throttle = 0; // Forward/Backward speed target (-255 to 255)
  int steering = 0; // Left/Right turning target (-255 to 255)

  // --- Map Y-axis (Throttle) ---
  if (myData.y_tilt > deadband) {
    throttle = map(myData.y_tilt, deadband, 90, 0, 255);
  } else if (myData.y_tilt < -deadband) {
    throttle = map(myData.y_tilt, -deadband, -90, 0, -255);
  }

  // --- Map X-axis (Steering) ---
  if (myData.x_tilt > deadband) {
    steering = map(myData.x_tilt, deadband, 90, 0, 255);
  } else if (myData.x_tilt < -deadband) {
    steering = map(myData.x_tilt, -deadband, -90, 0, -255);
  }

  // --- Differential Drive Mixing ---
  int leftSpeed = throttle + steering;
  int rightSpeed = throttle - steering;

  // Constrain speeds to prevent PWM overflow (values over 255 wrap back to 0)
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);

  // --- Drive Left Motor ---
  if (leftSpeed > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, leftSpeed);
  } else if (leftSpeed < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, abs(leftSpeed)); // analogWrite must always be positive
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0);
  }

  // --- Drive Right Motor ---
  if (rightSpeed > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, rightSpeed);
  } else if (rightSpeed < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENB, abs(rightSpeed)); 
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, 0);
  }
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
  
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENB, 0);
}

