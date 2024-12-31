#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "web.h"
#include <esp_task_wdt.h>
#include <WiFiClient.h>
#include <vector>
#include <ESPmDNS.h>
#include <ElegantOTA.h>

#define NUMBERDEVICE 10
#define CURRENT_DEVICE 6

const char* deviceIPs[NUMBERDEVICE] = {
  "192.168.4.165",
  "192.168.4.166",
  "192.168.4.167",
  "192.168.4.168",
  "192.168.4.169",
  "192.168.4.170"
};

// Replace with your network credentials
const char* ssid = "TTD_Quat";
const char* password = "TTD.2022";

const char* PARAM_INPUT_1 = "fan";
const char* PARAM_INPUT_2 = "action";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebServer OTA(8080);

const int serverPort = 5000;
WiFiServer tcpServer(serverPort);
// Vector to store connected clients
std::vector<WiFiClient> clients; 

int fanValues[NUMBERDEVICE] = {0, 0, 0, 0, 0, 0};
bool connectStatus[NUMBERDEVICE] = {false, false, false, false, false, false};

const unsigned long HEARTBEAT_TIMEOUT = 5000;
const unsigned long REQUIRE_HEARTBEAT_TIMEOUT = 2000;
unsigned long lastHeartbeatTime[NUMBERDEVICE] = {0, 0, 0, 0, 0, 0};
unsigned long lastCheckTime = 0;

// Web button
String status;
String processor(const String& var){
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    for (int i = 0; i < NUMBERDEVICE-6; i++) {
      status = connectStatus[i] ? " : Đã kết nối" : " : Chưa kết nối";
      buttons += "<h4>Quạt " + String(i+1) + status + "</h4>";
      buttons += "<h4>Mức: "+ String(fanValues[i])+"</h4>";
      buttons += "<button class=\"button button-decrease\" onclick=\"sendAction(" + String(i) + ", 'decrease')\">Giảm tốc độ</button>";
      buttons += "<button class=\"button button-increase\" onclick=\"sendAction(" + String(i) + ", 'increase')\">Tăng tốc độ</button><br><br>";
    }
    return buttons;
  }
  return String();
}

// Create message
String create_Message(int fanIndex, int value) {
  // Create the data string
  String id = "FAN" + String(fanIndex + 1);
  String val = String(value);
  String data = id + ":" + val;

  // Calculate XOR checksum
  uint8_t checksum = 0;
  for (size_t i = 0; i < data.length(); i++) {
    checksum ^= data[i];
  }

  // Convert checksum to hexadecimal string
  char checksumStr[3]; // Two hex digits + null terminator
  sprintf(checksumStr, "%02X", checksum);

  // Create the message with the checksum
  String message = "#" + data + "*" + checksumStr + "%";
  return message;
}

// Function to create heartbeat message
String createHeartbeatMessage(int i) {
  String message = "HEARTBEAT"+ String(i); // No data in this message, just the heartbeat identifier
  // Calculate the XOR checksum for this message
  uint8_t checksum = 0;
  for (size_t i = 0; i < message.length(); i++) {
    checksum ^= message[i];
  }

  // Convert checksum to hexadecimal string
  char checksumStr[3]; // Two hex digits + null terminator
  sprintf(checksumStr, "%02X", checksum);

  // Update the message with the correct checksum
  message = "#" +message +"*" + String(checksumStr) +"%";
  return message;
}


// Broadcast message
void broadcast_Command(String message){
  for (auto& client : clients) {
    if (client.connected()) {
      client.println(message); // send to station
    }
  }
}

// static IP 
IPAddress local_ip(192, 168, 4, 115); // static IP for AP mode
IPAddress gateway(192, 168, 4, 1);    // Gateway
IPAddress subnet(255, 255, 255, 0);   // Subnet Mask 

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Set up the ESP32 as an access point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  // WiFi.softAP(ssid, password);
  WiFi.softAP(ssid, password, 1, 0, 10);
  delay(100); // Give some time for the AP to start

  // Print the IP address of the AP
  Serial.println("Access Point IP: ");
  Serial.println(WiFi.softAPIP());

  // OTA
  ElegantOTA.begin(&OTA);    // Start ElegantOTA
  OTA.begin();
  // Serial.println("HTTP server started");

  // Start TCP server
  tcpServer.begin();
  
  // Start mDNS responder and set domain
  if (!MDNS.begin("ttd")) { // "TTD.local"
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started. Domain: TTD.local");
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?fan=<inputMessage1>&action=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?fan=<inputMessage1>&action=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      int fanIndex = inputMessage1.toInt();
      int val;
      if (inputMessage2 == "increase") {
        val = (fanValues[fanIndex] == 9) ? fanValues[fanIndex] : (fanValues[fanIndex] + 1);
      } else if (inputMessage2 == "decrease") {
        val = (fanValues[fanIndex] == 0) ? fanValues[fanIndex] : (fanValues[fanIndex] - 1);
      }
      String message = create_Message(fanIndex, val);
      broadcast_Command(message);
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "[";
    for (int i = 0; i < NUMBERDEVICE-6; i++) {
      json += "{\"fan\":" + String(i+1) + ",\"status\":\"" + (connectStatus[i] ? "Đã kết nối" : "Chưa kết nối") + "\",\"level\":" + String(fanValues[i]) + "}";
      if (i < NUMBERDEVICE - 1) json += ",";
    }
    json += "]";
    request->send(200, "application/json", json);
  });

  // Start server
  server.begin();
}

bool verifyXORChecksum(const String& message) {
  // Find the position of the '*' character
  int starPos = message.indexOf('*');
  if (starPos == -1) {
    return false; // Invalid message format
  }

  // Extract the data part and the checksum part
  String data = message.substring(1, starPos); // Exclude the '#' character
  String receivedChecksumStr = message.substring(starPos + 1, message.length() - 1);

  // Convert the received checksum from hexadecimal string to integer
  uint8_t receivedChecksum = strtol(receivedChecksumStr.c_str(), NULL, 16);

  // Calculate the XOR checksum for the data part
  uint8_t calculatedChecksum = 0;
  for (size_t i = 0; i < data.length(); i++) {
    calculatedChecksum ^= data[i];
  }

  // Compare the calculated checksum with the received checksum
  return (calculatedChecksum == receivedChecksum);
}

// Update fan value from station 
void updateFanValue(String& message) {
  int idPos = message.indexOf(':');
  String device = message.substring(1, idPos);

  int val = int(message[message.indexOf('*') - 1]) - 48;

  // Parse fan index from the device name (e.g., "FAN1")
  if (device.startsWith("FAN")) {
    int fanIndex = device.substring(3).toInt() - 1; // Extract fan number and convert to 0-based index
    fanValues[fanIndex] = val;
    Serial.println("Update: FAN" + String(fanIndex + 1) + ":" + String(val));
  }
}

// Heartbeat
void handleHeartbeat(String& message) {
  int pos = message.indexOf('*');
  int id = int(message[pos - 1]) - 1 - '0';
  lastHeartbeatTime[id] = millis();
  connectStatus[id] = true;
}

// Handle data
void handleCommand(String& message) {
  // Verify the XOR checksum  
  if (verifyXORChecksum(message)) {
    if (message.startsWith("#HEARTBEAT")) {
      handleHeartbeat(message);
    } else {
      updateFanValue(message);
    }
  }
}

// Check if connected
bool isClientConnected(const char* ip) {
  for (auto& client : clients) {
    if (client.connected() && client.remoteIP().toString() == String(ip)) {
      return true;
    }
  }
  return false;
}

void loop() {
  // Handle new TCP client connections
  WiFiClient client = tcpServer.available();
  if (client) {
    clients.push_back(client);
  }

  // Check for data from clients and remove disconnected clients
  for (auto it = clients.begin(); it != clients.end(); ) {
    Serial.println(it->remoteIP());
    if (it->connected()) {
      if (it->available()) {
        String response = it->readStringUntil('\n'); // Read data
        handleCommand(response);
        Serial.println("Received from station: " + response);
      }
      ++it;
    }  
    else {
      Serial.println("Disconnect: " + it->remoteIP());
      it = clients.erase(it);
    }
  }

  // Check if the specific clients are connected
  for (int i = 0; i < CURRENT_DEVICE+1; i++) {
    if (millis() - lastHeartbeatTime[i] > HEARTBEAT_TIMEOUT) {
      connectStatus[i] = false;
      fanValues[i] = 0;
    } else {
      connectStatus[i] = true;
    }
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastCheckTime >= REQUIRE_HEARTBEAT_TIMEOUT) {
    lastCheckTime = currentMillis;
    for (int i = 0; i < CURRENT_DEVICE+1; i++){
      broadcast_Command(createHeartbeatMessage(i));
      delay(100);
    }
  }
}