#ifndef GLOBAL_H
#define GLOBAL_H

#define NUMBERDEVICE    12
#define CURRENT_DEVICE  6

// reset when disconnect
unsigned long RESET_TIMEOUT = 60;
unsigned long lastResetTime = 0;

// Define LED pin
#define LED5  5
#define LED17 17 
#define LED16 16 
#define LED4  4   
const int ledPins[] = {LED5, LED17, LED16, LED4, LED4, LED4};

// Define ADC pin
#define ADC32 32   // Fan 3
#define ADC33 33   // Fan 4
#define ADC34 34   // Fan 1
#define ADC35 35   // Fan 2
const int adcPins[] = {ADC34, ADC35, ADC32, ADC33, ADC35, ADC35}; 


// Replace with your network credentials
const char* ssid = "TTD_Quat";
const char* password = "TTD.2022";

// Static IP 
IPAddress local_ip(192, 168, 4, 115); // Static IP for AP mode
IPAddress gateway(192, 168, 4, 1);    // Gateway
IPAddress subnet(255, 255, 255, 0);   // Subnet Mask 

// Create AsyncWebServer object on port 8080 for OTA
const int serverPort = 5000;
WiFiServer tcpServer(serverPort);
AsyncWebServer OTA(8080);

// // Static client IP
// IPAddress deviceIPs[CURRENT_DEVICE] = {
//   IPAddress(192, 168, 4, 165),  
//   IPAddress(192, 168, 4, 166),  
//   IPAddress(192, 168, 4, 167),  
//   IPAddress(192, 168, 4, 168),  
// };

// Check if OTA success
bool OTAStatus = false;

// Client
WiFiClient* clients[NUMBERDEVICE];

// Fan value and connect status
int fanValues[NUMBERDEVICE];
bool connectStatus[NUMBERDEVICE];

// Heartbeat check connection
unsigned long currentMillis = 0;
const unsigned long HEARTBEAT_TIMEOUT = 15000;
const unsigned long REQUIRE_HEARTBEAT_TIMEOUT = 3000;

//get first heartbeat - to check connection
long lastHeartbeatTime[NUMBERDEVICE];
unsigned long lastCheckTime = 0;
bool nextHeartbeat = true;
int curHeartbeat = 1;

// check for reduce noise, range +-40
int previousLevels[CURRENT_DEVICE];
int lastADCValues[CURRENT_DEVICE];
int ADC_samples[CURRENT_DEVICE];
int ADC_values[CURRENT_DEVICE][10];

const unsigned long ADCsample = 10;
unsigned long lastADC = 0;
int countgetADC = 0; 
int countsendADC = 0; 

// number server connect to wifi, max 16
int serverConnected= 0;

void setupGlobalVariable(){
  for(int i =0; i< CURRENT_DEVICE; i++){
    fanValues[i] = 0;
    connectStatus[i] = false;
    // lastHeartbeatTime[i] = -15000;
    lastHeartbeatTime[i] = 0;
    previousLevels[i] = -1;
    lastADCValues[i]= 0;
    ADC_samples[i]= 0;
  }
}

void setupClient(){
  for (int i = 0; i < NUMBERDEVICE; i++) {
    clients[i] = nullptr;
  }
}

void setupLed(){
  for (int i = 0; i <CURRENT_DEVICE; i++) {
    pinMode(ledPins[i], OUTPUT);   // Set each pin as OUTPUT
    digitalWrite(ledPins[i], LOW); // Set each LED initially to OFF
  }
}

#endif