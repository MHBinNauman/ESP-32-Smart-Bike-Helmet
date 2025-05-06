////////////////// Receiver Code //////////////////////
#include <WiFi.h>
#include <esp_now.h>

// Replace this with the actual MAC address of the sender ESP32
uint8_t senderAddress[] = {0xC0, 0x5D, 0x89, 0xB1, 0x13, 0x60};  // <-- Put actual MAC here

const int relayPin = 23;

void setup() {
  Serial.begin(115200);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);  // Relay OFF (active LOW)

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  // Add sender peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, senderAddress, 6);
  peerInfo.channel = 0;  // Use current channel
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  // Register callback to receive data
  esp_now_register_recv_cb(onDataReceive);
}

// Updated receive callback
void onDataReceive(const esp_now_recv_info_t *recvInfo, const uint8_t *data, int len) {
  Serial.print("Received from: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", recvInfo->src_addr[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  if (len > 0) {
    char command = (char)data[0];
    Serial.print("Received Command: ");
    Serial.println(command);

    if (command == 0t) {
      digitalWrite(relayPin, LOW);  // ON
      Serial.println("Relay ON");
    } else {
      digitalWrite(relayPin, HIGH); // OFF
      Serial.println("Relay OFF");
    }
  }
}

void loop() {
  // Nothing here
}
