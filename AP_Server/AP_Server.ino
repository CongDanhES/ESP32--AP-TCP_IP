// #include <WiFi.h>
// #include <AsyncTCP.h>
// #include <ESPAsyncWebServer.h>
// #include "web.h"
// #include <esp_task_wdt.h>
// #include <WiFiClient.h>
// #include <vector>

// // Replace with your network credentials
// const char* ssid = "ESP32_AP";
// const char* password = "12345678";

// const char* PARAM_INPUT_1 = "fan";
// const char* PARAM_INPUT_2 = "action";

// // Create AsyncWebServer object on port 80
// AsyncWebServer server(80);

// const int serverPort = 5000;
// WiFiServer tcpServer(serverPort);
// std::vector<WiFiClient> clients; // Vector to store connected clients

// int fanValues[6] = {0, 0, 0, 0, 0, 0}; // Values for FAN1, FAN2, FAN3, FAN4, FAN5, FAN6

// // Web button
// String processor(const String& var){
//   if(var == "BUTTONPLACEHOLDER"){
//     String buttons = "";
//     for (int i = 0; i < 6; i++) {
//       buttons += "<h4>FAN" + String(i+1) + "</h4>";
//       buttons += "<button class=\"button button-decrease\" onclick=\"sendAction(" + String(i) + ", 'decrease')\">Giảm tốc độ</button>";
//       buttons += "<button class=\"button button-increase\" onclick=\"sendAction(" + String(i) + ", 'increase')\">Tăng tốc độ</button><br><br>";
//     }
//     return buttons;
//   }
//   return String();
// }

// // static IP 
// IPAddress local_ip(192, 168, 4, 115); // static IP for AP mode
// IPAddress gateway(192, 168, 4, 1);    // Gateway
// IPAddress subnet(255, 255, 255, 0);   // Subnet Mask 

// //broadcast message
// void broadcast_Command(String message){
//   for (auto& client : clients) {
//     if (client.connected()) {
//       client.println(message); // send to station
//     }
//   }
// }

// //create message
// String create_Message(int fanIndex, int value) {
//   String id = "FAN" + String(fanIndex + 1);
//   String val = String(value);
//   uint8_t checksum = (id[0] ^ id[1] ^ id[2] ^ id[3] ^ val[0]); 
//   String message = "#" + id + ":" + val +String(checksum)+ "%";
//   return message;
// }

// void setup(){
//   // Serial port for debugging purposes
//   Serial.begin(115200);

//   // Set up the ESP32 as an access point
//   WiFi.mode(WIFI_AP);
//   WiFi.softAPConfig(local_ip, gateway, subnet);
//   WiFi.softAP(ssid, password);
//   delay(100); // Give some time for the AP to start


//   // Print the IP address of the AP
//   Serial.println("Access Point IP: ");
//   Serial.println(WiFi.softAPIP());

//   // Start TCP server
//   tcpServer.begin();

//   // Route for root / web page
//   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
//     request->send(200, "text/html", index_html, processor);
//   });

//   // Send a GET request to <ESP_IP>/update?fan=<inputMessage1>&action=<inputMessage2>
//   server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
//     String inputMessage1;
//     String inputMessage2;
//     // GET input1 value on <ESP_IP>/update?fan=<inputMessage1>&action=<inputMessage2>
//     if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
//       inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
//       inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
//       int fanIndex = inputMessage1.toInt();
//       if (inputMessage2 == "increase" && fanValues[fanIndex] < 9) {
//         fanValues[fanIndex]++;
//         String message = create_Message(fanIndex, fanValues[fanIndex]);
//         broadcast_Command(message);
//       } else if (inputMessage2 == "decrease" && fanValues[fanIndex] > 0) {
//         fanValues[fanIndex]--;
//         String message = create_Message(fanIndex, fanValues[fanIndex]);
//         broadcast_Command(message);
//       }

//       // Print current values to Serial
//       Serial.print("FAN1 - Value: ");
//       Serial.print(fanValues[0]);
//       Serial.print(", FAN2 - Value: ");
//       Serial.print(fanValues[1]);
//       Serial.print(", FAN3 - Value: ");
//       Serial.print(fanValues[2]);
//       Serial.print(", FAN4 - Value: ");
//       Serial.print(fanValues[3]);
//       Serial.print(", FAN5 - Value: ");
//       Serial.print(fanValues[4]);
//       Serial.print(", FAN6 - Value: ");
//       Serial.println(fanValues[5]);
//     }
//     else {
//       inputMessage1 = "No message sent";
//       inputMessage2 = "No message sent";
//     }
//     request->send(200, "text/plain", "OK");
//   });

//   // Start server
//   server.begin();
// }

// void loop() {
  
//   // Handle new TCP client connections
//   WiFiClient client = tcpServer.available();
//   if (client) {
//     clients.push_back(client);
//   }

//   // Check for data from clients and remove disconnected clients
//   for (auto it = clients.begin(); it != clients.end(); ) {
//     if (it->connected()) {
//       if (it->available()) {
//         String response = it->readStringUntil('\n'); // Đọc dữ liệu phản hồi
//         Serial.println("Received from station: " + response);
//       }
//       ++it;
//     } else {
//       it = clients.erase(it); // Remove disconnected client
//     }
//   }
// }
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "web.h"
#include <esp_task_wdt.h>
#include <WiFiClient.h>
#include <vector>

// Replace with your network credentials
const char* ssid = "ESP32_AP";
const char* password = "12345678";

const char* PARAM_INPUT_1 = "fan";
const char* PARAM_INPUT_2 = "action";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const int serverPort = 5000;
WiFiServer tcpServer(serverPort);
std::vector<WiFiClient> clients; // Vector to store connected clients

int fanValues[6] = {0, 0, 0, 0, 0, 0}; // Values for FAN1, FAN2, FAN3, FAN4, FAN5, FAN6

// Web button
String processor(const String& var){
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    for (int i = 0; i < 6; i++) {
      buttons += "<h4>FAN" + String(i+1) + "</h4>";
      buttons += "<button class=\"button button-decrease\" onclick=\"sendAction(" + String(i) + ", 'decrease')\">Giảm tốc độ</button>";
      buttons += "<button class=\"button button-increase\" onclick=\"sendAction(" + String(i) + ", 'increase')\">Tăng tốc độ</button><br><br>";
    }
    return buttons;
  }
  return String();
}

// static IP 
IPAddress local_ip(192, 168, 4, 115); // static IP for AP mode
IPAddress gateway(192, 168, 4, 1);    // Gateway
IPAddress subnet(255, 255, 255, 0);   // Subnet Mask 

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

// Create message
String create_Message(int fanIndex, int value) {
  String id = "FAN" + String(fanIndex + 1);
  String val = String(value);
  String data = id + ":" + val;

  // Calculate CRC-16 checksum
  uint16_t checksum = crc16((const uint8_t*)data.c_str(), data.length());

  // Convert checksum to hexadecimal string
  char checksumStr[5];
  sprintf(checksumStr, "%04X", checksum);

  // Create the message with the checksum
  String message = "#" + data + "*" + checksumStr + "%";
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

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Set up the ESP32 as an access point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password);
  delay(100); // Give some time for the AP to start

  // Print the IP address of the AP
  Serial.println("Access Point IP: ");
  Serial.println(WiFi.softAPIP());

  // Start TCP server
  tcpServer.begin();

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
      if (inputMessage2 == "increase" && fanValues[fanIndex] < 9) {
        fanValues[fanIndex]++;
        String message = create_Message(fanIndex, fanValues[fanIndex]);
        broadcast_Command(message);
      } else if (inputMessage2 == "decrease" && fanValues[fanIndex] > 0) {
        fanValues[fanIndex]--;
        String message = create_Message(fanIndex, fanValues[fanIndex]);
        broadcast_Command(message);
      }

      // Print current values to Serial
      Serial.print("FAN1 - Value: ");
      Serial.print(fanValues[0]);
      Serial.print(", FAN2 - Value: ");
      Serial.print(fanValues[1]);
      Serial.print(", FAN3 - Value: ");
      Serial.print(fanValues[2]);
      Serial.print(", FAN4 - Value: ");
      Serial.print(fanValues[3]);
      Serial.print(", FAN5 - Value: ");
      Serial.print(fanValues[4]);
      Serial.print(", FAN6 - Value: ");
      Serial.println(fanValues[5]);
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();
}

void loop() {
  // Handle new TCP client connections
  WiFiClient client = tcpServer.available();
  if (client) {
    clients.push_back(client);
  }

  // Check for data from clients and remove disconnected clients
  for (auto it = clients.begin(); it != clients.end(); ) {
    if (it->connected()) {
      if (it->available()) {
        String response = it->readStringUntil('\n'); // Read data
        Serial.println("Received from station: " + response);
      }
      ++it;
    } else {
      it = clients.erase(it); // Remove disconnected client
    }
  }
}