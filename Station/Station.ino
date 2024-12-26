#include <WiFi.h>

const char* ssid = "ESP32_AP";
const char* password = "12345678";

const char* server_ip = "192.168.4.115"; // IP address of the ESP32 server in AP mode
const uint16_t server_port = 5000; // Port of the ESP32 server

// Static IP configuration for the client
IPAddress local_ip(192, 168, 4, 165); // Static IP for the client
IPAddress gateway(192, 168, 4, 1);  // Gateway IP
IPAddress subnet(255, 255, 255, 0); // Subnet Mask

WiFiClient client;

// CRC-16-CCITT polynomial
const uint16_t CRC_POLY = 0x1021;
const uint16_t CRC_INIT = 0xFFFF;

uint16_t crc16(const uint8_t* data, size_t length) {
  uint16_t crc = CRC_INIT;
  for (size_t i = 0; i < length; i++) {
    crc ^= (data[i] << 8);
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ CRC_POLY;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

void connectToWiFi() {
  // Connect to Wi-Fi
  WiFi.config(local_ip, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println("Connected to WiFi ");
  Serial.print("ESP32 Client IP Address: ");
  Serial.println(WiFi.localIP());
}

void connectToServer() {
  // Connect to the server
  while (!client.connect(server_ip, server_port)) {
    Serial.println("Failed to connect to server, retrying...");
    delay(1000);
  }
  Serial.println("Connected to server");
}

bool verifyCRC16(const String& message) {
  // Find the position of the '*' character
  int starPos = message.indexOf('*');
  if (starPos == -1) {
    return false; // Invalid message format
  }

  // Extract the data part and the checksum part
  String data = message.substring(1, starPos); // Exclude the '#' character
  String receivedChecksumStr = message.substring(starPos + 1, message.length() - 1); 

  // Convert the received checksum from hexadecimal string to integer
  uint16_t receivedChecksum = strtol(receivedChecksumStr.c_str(), NULL, 16);

  // Calculate the CRC-16 checksum for the data part
  uint16_t calculatedChecksum = crc16((const uint8_t*)data.c_str(), data.length());

  // Compare the calculated checksum with the received checksum
  return (calculatedChecksum == receivedChecksum);
}

void setPWM(String& message){
  int PWM = int(message[message.indexOf('*')-1])- 48;
  Serial.println(PWM);
}

void handleCommand(String& message){

  String device;

  // Verify the CRC-16 checksum  
  if (verifyCRC16(message)) {
    Serial.println("Checksum valid");

    int idPos = message.indexOf(':');
    device = message.substring(1, idPos); 
    if(device== "FAN1"){
      setPWM(message);
      Serial.println("Set PWM");
    }
  } else {
    Serial.println("Checksum invalid");
  }
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Connect to Wi-Fi
  connectToWiFi();

  // Connect to the server
  connectToServer();
}

void loop() {
  // Reconnect to Wi-Fi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    connectToWiFi();
  }

  // Reconnect to the server if disconnected
  if (!client.connected()) {
    Serial.println("Server disconnected, reconnecting...");
    connectToServer();
  }

  // Handle communication with the server
  while (client.connected()) {
    if (client.available()) {
      String message = client.readStringUntil('\n'); // Read data
      Serial.println("Received data: " + message);
      handleCommand(message);
    }
  }
}