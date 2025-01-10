#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiClient.h>
#include <ElegantOTA.h>

#define NUMBERDEVICE    12
#define CURRENT_DEVICE  4

// Led for connect status 1, 2, 3, 4
#define LED5  5
#define LED17 17 
#define LED16 16
#define LED4  4   
const int ledPins[] = {LED5, LED17, LED16, LED4};

// Replace with your network credentials
const char* ssid = "TTD_Quat";
const char* password = "TTD.2022";

// Create AsyncWebServer object on port 8080 for OTA
AsyncWebServer OTA(8080);
const int serverPort = 5000;
WiFiServer tcpServer(serverPort);

// Check if OTA success
bool OTAStatus = false;

// Client
WiFiClient* clients[NUMBERDEVICE];

// Fan value and connect status
int fanValues[NUMBERDEVICE] = {0, 0, 0, 0};
bool connectStatus[NUMBERDEVICE] = {false, false, false, false};

// Heartbeat check connection
unsigned long currentMillis = 0;
const unsigned long HEARTBEAT_TIMEOUT = 15000;
const unsigned long REQUIRE_HEARTBEAT_TIMEOUT = 3000;

//get first heartbeat negative -15000 to check connection
long lastHeartbeatTime[NUMBERDEVICE] = {-15000, -15000, -15000, -15000};
unsigned long lastCheckTime = 0;
bool nextHeartbeat = true;
int curHeartbeat = 1;

// Static client IP
IPAddress deviceIPs[CURRENT_DEVICE] = {
  IPAddress(192, 168, 4, 165),  
  IPAddress(192, 168, 4, 166),  
  IPAddress(192, 168, 4, 167),  
  IPAddress(192, 168, 4, 168),  
};

// ADC
#define ADC32 32   // Fan 3
#define ADC33 33   // Fan 4
#define ADC34 34   // Fan 1
#define ADC35 35   // Fan 2

int ADC_sample1 = 0;
int ADC_sample2 = 0;
int ADC_sample3 = 0;
int ADC_sample4 = 0;

uint16_t ADC_value1[10];
uint16_t ADC_value2[10];
uint16_t ADC_value3[10];
uint16_t ADC_value4[10];

const unsigned long ADCsample = 10;
unsigned long lastADC = 0;

int countgetADC = 0; 
int countsendADC = 0; 

int lastADCValue1 = 0;
int lastADCValue2 = 0;
int lastADCValue3 = 0;
int lastADCValue4 = 0;

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
  String message = "HEARTBEAT" + String(i); // No data in this message, just the heartbeat identifier
  // Calculate the XOR checksum for this message
  uint8_t checksum = 0;
  for (size_t i = 0; i < message.length(); i++) {
    checksum ^= message[i];
  }

  // Convert checksum to hexadecimal string
  char checksumStr[3]; // Two hex digits + null terminator
  sprintf(checksumStr, "%02X", checksum);

  // Update the message with the correct checksum
  message = "#" + message + "*" + String(checksumStr) + "%";
  return message;
}

// Broadcast message
void broadcast_Command(String message) {
  for (int i = 0; i < NUMBERDEVICE; i++) {
    if (clients[i] != nullptr && clients[i]->connected()) {
      clients[i]->println(message); // Send to client
    }
  }
}

// Static IP 
IPAddress local_ip(192, 168, 4, 115); // Static IP for AP mode
IPAddress gateway(192, 168, 4, 1);    // Gateway
IPAddress subnet(255, 255, 255, 0);   // Subnet Mask 

void setup() {

  // output led
  pinMode(LED5, OUTPUT);  
  pinMode(LED17, OUTPUT);  
  pinMode(LED16, OUTPUT);
  pinMode(LED4, OUTPUT);    
  digitalWrite(LED5, LOW);
  digitalWrite(LED17, LOW);
  digitalWrite(LED16, LOW);
  digitalWrite(LED4, LOW);

  // Serial port for debugging purposes
  Serial.begin(115200);

  // Set up the ESP32 as an access point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password, 1, 0, 20);
  // WiFi.softAP(ssid, password, 1, 1, 10);
  delay(100); // Give some time for the AP to start

  // Print the IP address of the AP
  Serial.println("Access Point IP: ");
  Serial.println(WiFi.softAPIP());

  // OTA
  OTA.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is ElegantOTA AsyncDemo.");
  });
  ElegantOTA.begin(&OTA);    // Start ElegantOTA
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  OTA.begin();

  // Start TCP server
  tcpServer.begin();

  // Initialize clients array
  for (int i = 0; i < NUMBERDEVICE; i++) {
    clients[i] = nullptr;
  }

}

// Checksum 
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
      nextHeartbeat = true;
      handleHeartbeat(message);
    } else {
      updateFanValue(message);
    }
  }
}

// OTA
unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    OTAStatus = true;
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

// Get level from ADC
int getLevelFan(int ADC) {
  if (ADC >= 0 && ADC < 100) return 0;
  else if (ADC >= 100 && ADC < 550) return 1;
  else if (ADC >= 550 && ADC < 1000) return 2;
  else if (ADC >= 1000 && ADC < 1450) return 3;
  else if (ADC >= 1450 && ADC < 1900) return 4;
  else if (ADC >= 1900 && ADC < 2350) return 5;
  else if (ADC >= 2350 && ADC < 2800) return 6;
  else if (ADC >= 2800 && ADC < 3250) return 7;
  else if (ADC >= 3250 && ADC < 3700) return 8;
  else if (ADC >= 3700) return 9;
  return -1;
}

// update led status for check connection
void updateLedStatus(int led, int status) {
  // Validate the input (ensure led is between 1 and 4)
  if (led >= 1 && led <= 4) {
    // Map led index (1-based) to array index (0-based)
    int pin = ledPins[led - 1];
    digitalWrite(pin, status == 1 ? HIGH : LOW);
  }
}

// check for reduce noise, range +-30
int previousLevel1 = -1;
int previousLevel2 = -1;
int previousLevel3 = -1;
int previousLevel4 = -1;
int updateFanLevel(int ADC_sample, int &previousLevel, int &lastADCValue) {
  int level = getLevelFan(ADC_sample);
  if (previousLevel != -1) {
    int border = previousLevel * 450 + 100;
    if (abs(ADC_sample - border) <= 40 && abs(lastADCValue - border) <= 40) {
      level = previousLevel;
    }
    // Serial.println(String(lastADCValue)+"-"+String(ADC_sample));
  }
  return level;
}

void loop() {
  // Handle new TCP client connections
  WiFiClient new_client = tcpServer.available();

  if (new_client) {
    bool added = false;
    for (int i = 0; i < NUMBERDEVICE; i++) {
      if (clients[i] == nullptr || !clients[i]->connected()) {
        clients[i] = new WiFiClient(new_client); // Allocate new WiFiClient dynamically
        Serial.print("New client connected: ");
        Serial.print(new_client.remoteIP());
        Serial.println(" at: "+ String(i));
        added = true;
        break;
      }
    }
    if (!added) {
      Serial.println("Max clients reached. Connection rejected.");
      new_client.stop();
    }
  }

  // Handle data from clients and remove disconnected clients
  for (int i = 0; i < NUMBERDEVICE; i++) {
    if (clients[i] != nullptr) {
      if (clients[i]->connected()) {
        if (clients[i]->available()) {
          String response = clients[i]->readStringUntil('\n'); // Read data
          handleCommand(response);  // Process command
          Serial.println("Received from station: " + response);
        }
      }
    }
  }

  // Check if the specific clients are connected 
  for (int i = 0; i < CURRENT_DEVICE + 1; i++) {
    if (millis() - lastHeartbeatTime[i] > HEARTBEAT_TIMEOUT) {
      connectStatus[i] = false;
      fanValues[i] = 0; 
      updateLedStatus(i+1,false);
      // removeUnconnectedClients();
    } else {
      connectStatus[i] = true;
      updateLedStatus(i+1,true);
    }
  }

  // If OTA success, upload new code, reset ESP
  if (OTAStatus) {
    ElegantOTA.loop();
  }

  // Send heartbeat to check connection
  currentMillis = millis();
  if ((currentMillis - lastCheckTime >= REQUIRE_HEARTBEAT_TIMEOUT) || nextHeartbeat) {
    if ((currentMillis - lastCheckTime >= REQUIRE_HEARTBEAT_TIMEOUT))
      nextHeartbeat = true;
    lastCheckTime = currentMillis;
    if (nextHeartbeat) {
      nextHeartbeat = false;
      broadcast_Command(createHeartbeatMessage(curHeartbeat));
      curHeartbeat++;
      curHeartbeat = (curHeartbeat > CURRENT_DEVICE+1) ? 1 : curHeartbeat;
    }
  }
  
  // ADC
  currentMillis = millis();
  if (currentMillis - lastADC >= ADCsample) {
    lastADC = currentMillis;
    ADC_value1[countgetADC] = analogRead(ADC34);
    ADC_value2[countgetADC] = analogRead(ADC35);
    ADC_value3[countgetADC] = analogRead(ADC32);
    ADC_value4[countgetADC] = analogRead(ADC33);
    countgetADC++;
  }

  if (countgetADC == 10) {
    for (int i = 0; i < countgetADC; i++) {
      ADC_sample1 += ADC_value1[i];
      ADC_sample2 += ADC_value2[i];
      ADC_sample3 += ADC_value3[i];
      ADC_sample4 += ADC_value4[i];
    }

    ADC_sample1 /= 10;
    ADC_sample2 /= 10;
    ADC_sample3 /= 10;
    ADC_sample4 /= 10;

    countsendADC++;
    if (countsendADC == 10) {
      // get level and reduce noise
      int level1 = updateFanLevel(ADC_sample1, previousLevel1, lastADCValue1);
      int level2 = updateFanLevel(ADC_sample2, previousLevel2, lastADCValue2);
      int level3 = updateFanLevel(ADC_sample3, previousLevel3, lastADCValue3);
      int level4 = updateFanLevel(ADC_sample4, previousLevel4, lastADCValue4);

      Serial.println("FAN1: "+String(level1)+" Value: "+String(lastADCValue1)+"-"+String(ADC_sample1));
      Serial.println("FAN2: "+String(level2)+" Value: "+String(lastADCValue2)+"-"+String(ADC_sample2));
      Serial.println("FAN3: "+String(level3)+" Value: "+String(lastADCValue3)+"-"+String(ADC_sample3));
      Serial.println("FAN4: "+String(level4)+" Value: "+String(lastADCValue4)+"-"+String(ADC_sample4));

      // save last value 
      previousLevel1 = level1;
      previousLevel2 = level2;
      previousLevel3 = level3;
      previousLevel4 = level4;

      lastADCValue1 = ADC_sample1;
      lastADCValue2 = ADC_sample2;
      lastADCValue3 = ADC_sample3;
      lastADCValue4 = ADC_sample4;

      // broadcast control message
      String message1 = create_Message(0, level1);
      String message2 = create_Message(1, level2);
      String message3 = create_Message(2, level3);
      String message4 = create_Message(3, level4);
      broadcast_Command(message1);
      broadcast_Command(message2);
      broadcast_Command(message3);
      broadcast_Command(message4);

      countsendADC = 0;
    }
    lastADCValue1 = ADC_sample1;
    lastADCValue2 = ADC_sample2;
    lastADCValue3 = ADC_sample3;
    lastADCValue4 = ADC_sample4;
    countgetADC = 0;
    // reset for new cycle
    ADC_sample1 = 0;
    ADC_sample2 = 0;
    ADC_sample3 = 0;
    ADC_sample4 = 0;
  }
    // Đếm số lượng thiết bị kết nối vào WiFi AP
  // int connectedDevices = WiFi.softAPgetStationNum();
  // Serial.printf("Number of connected devices: %d\n", connectedDevices);
}