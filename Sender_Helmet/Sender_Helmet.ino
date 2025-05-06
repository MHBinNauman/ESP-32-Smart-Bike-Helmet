/////////////////////// Sender Code /////////////
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <esp_now.h>
#include <WiFi.h>
#include "BluetoothSerial.h"

// --- Bluetooth for App ---
BluetoothSerial SerialBT;

// --- Touch Sensor Pins ---
const int touch1Pin = 14;
const int touch2Pin = 27;

// --- MPU6050 ---
Adafruit_MPU6050 mpu;
double prev_ax = 0, prev_ay = 0, prev_az = 0;
unsigned long prev_time = 0;

// --- State ---
bool crashDetected = false;
int lastHelmetState = -1;

// --- ESP-NOW Receiver MAC ---
uint8_t receiverMAC[] = {0xC0,0x5D,0x89,0xB1,0x13,0x60};  // Replace with actual

// --- Data structure ---
typedef struct struct_message {
  uint8_t signal;  // 0 = helmet on, 2 = off, 1 = crash
} struct_message;

struct_message dataToSend;

void setup() {
  Serial.begin(115200);

  pinMode(touch1Pin, INPUT);
  pinMode(touch2Pin, INPUT);

  // Bluetooth for mobile app
  SerialBT.begin("CrashHelmet");

  // MPU6050 init
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found");
    while (1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  delay(100);
  prev_time = millis();

  // Wi-Fi and ESP-NOW init
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  int touch1 = digitalRead(touch1Pin);
  int touch2 = digitalRead(touch2Pin);
  int currentHelmetState = (touch1 == HIGH && touch2 == HIGH) ? 0 : 2;

  if (currentHelmetState != lastHelmetState) {
    SerialBT.write((uint8_t)currentHelmetState);
    dataToSend.signal = currentHelmetState;
    esp_now_send(receiverMAC, (uint8_t*)&dataToSend, sizeof(dataToSend));
    lastHelmetState = currentHelmetState;

    if (currentHelmetState == 0)
      crashDetected = false;
  }

  // --- Crash detection ---
  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  double a_x = accel.acceleration.x;
  double a_y = accel.acceleration.y;
  double a_z = accel.acceleration.z;
  double resultant_acceleration = sqrt(a_x * a_x + a_y * a_y + a_z * a_z);

  unsigned long current_time = millis();
  double dt = (current_time - prev_time) / 1000.0;
  double delta_a = sqrt(pow(a_x - prev_ax, 2) + pow(a_y - prev_ay, 2) + pow(a_z - prev_az, 2));
  double jerk = delta_a / dt;

  prev_ax = a_x;
  prev_ay = a_y;
  prev_az = a_z;
  prev_time = current_time;

  double g_x = gyro.gyro.x;
  double g_y = gyro.gyro.y;
  double g_z = gyro.gyro.z;
  double resultant_g = sqrt(g_x * g_x + g_y * g_y + g_z * g_z);

  Serial.print("Acc: "); Serial.print(resultant_acceleration);
  Serial.print(" | Jerk: "); Serial.print(jerk);
  Serial.print(" | Angular: "); Serial.println(resultant_g);

  if (!crashDetected && resultant_acceleration > 39.2 && resultant_g > 3 && jerk > 100) {
    Serial.println("Crash Detected!");
    SerialBT.write((uint8_t)1);
    dataToSend.signal = 1;
    esp_now_send(receiverMAC, (uint8_t*)&dataToSend, sizeof(dataToSend));
    crashDetected = true;
  }

  delay(300);
}
