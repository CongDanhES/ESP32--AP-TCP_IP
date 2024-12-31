#include <WiFi.h>
#include <ElegantOTA.h>
#include <ESPmDNS.h>

#define DEVICE        0
#define FANID         "FAN1"
#define MDNS_NAME     "QUAT1"
#define PWM_PIN       2 
#define FREQ          1000  
#define RESOLUTION    8
#define HEARTBEAT     "HEARTBEAT1"
#define ESC_Pin       18 

#define MAX           1340
#define MIN           1100

// Static IP configuration for the client
IPAddress local_ip(192, 168, 4, 165); // Static IP for the client
IPAddress gateway(192, 168, 4, 1);  // Gateway IP
IPAddress subnet(255, 255, 255, 0); // Subnet Mask

const char* ssid = "TTD_Quat";
const char* password = "TTD.2022";

const char* server_ip = "192.168.4.115"; // IP address of the ESP32 server in AP mode
const uint16_t server_port = 5000; // Port of the ESP32 server

//Check
int serverConnect = 0;
int wifiConnect = 0;

WiFiClient client;

AsyncWebServer OTA(80);

// Value for current and tarrget PWM
int curValue = 0;
int tarValue = 0;

// Timestamp for the last heartbeat
unsigned long lastHeartbeatTime = 0; 
unsigned long heartbeatInterval = 2500;

//Setup for PWM:
int dutyCycle;
const int pwmChannel = 0;        
const int pwmFrequency = 50;     
const int pwmResolution = 16;    

//connect Accesspint
void connectToWiFi() {
  // Connect to Wi-Fi
  WiFi.config(local_ip, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    wifiConnect++;
    if(wifiConnect == 10){
      wifiConnect = 0;
      resetDisconnect();
    }
    Serial.println("Connecting to WiFi..");  
    delay(100);
  }

  // Print ESP Local IP Address
  Serial.println("Connected to WiFi ");
  Serial.print("ESP32 Client IP Address: ");
  Serial.println(WiFi.localIP());
}

void connectToServer() {
  // Connect to the server
  while (!client.connect(server_ip, server_port)) {
    serverConnect++;
    serverConnect++;
    if(serverConnect == 10){
      serverConnect = 0;
      resetDisconnect();
    }
    Serial.println("Failed to connect to server, retrying...");
    delay(100);
  }
  Serial.println("Connected to server");
}

// Check sum receive
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

// create message
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

// get& set PWM
void setPWM(int val){
  int duty;
  switch (val) {
    case 0:
      duty = getDuty(1000);
      break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      duty = getDuty(1100+ (val-1)*(MAX-MIN)/8);
      break;
  }
  ledcWrite(pwmChannel, duty);
  Serial.println(duty);
  String mess= create_Message(DEVICE,val);
  client.println(mess);
}

// get PWM value
void getPWM(String& message) {
  int PWM = int(message[message.indexOf('*') - 1]) - '0'; 
  setPWM(PWM);
  Serial.println(PWM);
}

//handle command

void handleHeartbeat(String& message) {
  // Serial.println("RECEIVE HEARTBEAT");
  int pos = message.indexOf('*');
  int id = int(message[pos - 1]) - 1 - '0';
  if(id == DEVICE){
    client.println(message);
  }
}

void handleCommand(String& message) {
  String device;

  // Verify the XOR checksum  
  if (verifyXORChecksum(message)) {
    Serial.println("Checksum valid");
    if (message.startsWith("#HEARTBEAT")) {
      handleHeartbeat(message);
    } 
    else{
      int idPos = message.indexOf(':');
      device = message.substring(1, idPos); 
      if (device == FANID) 
        getPWM(message);
    }
  }
  else {
    Serial.println("Checksum invalid");
  }
}


int getDuty(int pwmValue){
  return dutyCycle = (pwmValue * (1 << pwmResolution)) / 20000;
}

void setup() {

  // Attach the pin to the PWM channel
  ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
  ledcAttachPin(ESC_Pin, pwmChannel);

  //2000us / 20000us
  dutyCycle = getDuty(2000);
  Serial.println(dutyCycle);
  ledcWrite(pwmChannel, dutyCycle);
  delay(4000);  

  // 1000us / 20000us
  dutyCycle = getDuty(1000);
  ledcWrite(pwmChannel, dutyCycle);
  delay(4000);  

  // Serial port for debugging purposes
  Serial.begin(115200);

  // Connect to Wi-Fi
  connectToWiFi();

  // Connect to the server
  connectToServer();

  //MDNS:
  if (!MDNS.begin(MDNS_NAME)) { // "TTD.local"
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started.");
  }

  //OTA
  ElegantOTA.begin(&OTA);    // Start ElegantOTA
  OTA.begin();

}

void resetDisconnect(){
  ledcWrite(0, 0); 
  curValue = 0;
  tarValue = 0;
}

void loop() {

  //
  // dutyCycle = getDuty(1100);
  // ledcWrite(pwmChannel, dutyCycle); 

  // Reconnect to Wi-Fi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    // resetDisconnect();
    connectToWiFi();
  }
  else
    wifiConnect = 0;

  // Reconnect to the server if disconnected
  if (!client.connected()) {
    Serial.println("Server disconnected, reconnecting...");
    // resetDisconnect();
    connectToServer(); 
  }
  else
    serverConnect = 0;

  // Handle communication with the server
  while (client.connected()) {

    // unsigned long currentMillis = millis();
    // if (currentMillis - lastHeartbeatTime >= heartbeatInterval) {
    //   String heartbeatMessage = createHeartbeatMessage();
    //   client.println(heartbeatMessage); // Send heartbeat
    //   lastHeartbeatTime = currentMillis; // Update the timestamp
    //   // Serial.println("Heartbeat sent: " + heartbeatMessage);
    // }

    if (client.available()) {
      String message = client.readStringUntil('\n'); // Read data
      Serial.println("Received data: " + message);
      handleCommand(message);
    }
  }
}