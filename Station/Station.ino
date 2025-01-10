#include <WiFi.h>
#include <ElegantOTA.h>
// #include <ESPmDNS.h>
#include "define.h"

//Check
int serverConnect = 0;
int wifiConnect = 0;

WiFiClient client;

AsyncWebServer OTA(80);

//Setup for PWM:
int dutyCycle;
const int pwmChannel1 = 0;  
const int pwmChannel2 = 0;          
const int pwmFrequency = 50;     
const int pwmResolution = 16;    

// Timestamp for ESC
unsigned long LowTimeInterval= 0;
unsigned long curESCTime= 0;
unsigned long lastESCTime = 0; 
unsigned long ESCInterval = 5000;

//OTA status 
bool OTAStatus = false;

// time reconnect wifi, server
int timeWifi= 100;
int timeServer= 100;

int countResetWifi;
int countResetServer;

//Reset timeout
unsigned long RESET_TIMEOUT = 30;
unsigned long lastResetTime = 0;

//Initialize ESC
static unsigned long escStartTime = 0; 
static bool isEscStart = false;
bool escInitialize = true;

//get duty
int getDuty(int pwmValue){
  return dutyCycle = (pwmValue * (1 << pwmResolution)) / 20000;
}
int lastDuty = getDuty(1000);

//connect Accesspint
void connectToWiFi() {
  // Connect to Wi-Fi
  WiFi.config(LOCAL_IP, GATEWAY, SUBNET);
  WiFi.begin(SSID, PASSWORD);
  timeWifi = 100;
  while (WiFi.status() != WL_CONNECTED) {
    wifiConnect++;
    if(wifiConnect == 10){
      wifiConnect = 0;
      timeWifi = 1000;
      resetDisconnect();
      countResetWifi++;
      if(countResetWifi == 3){
        esp_restart();
      }
    }
    Serial.println("Connecting to WiFi..");  
    delay(timeWifi);
  }

  // Print ESP Local IP Address
  Serial.println("Connected to WiFi ");
  Serial.print("ESP32 Client IP Address: ");
  Serial.println(WiFi.localIP());
}

void connectToServer() {
  // Connect to the server
  timeServer = 100;
  while (!client.connect(SERVER_IP, SERVER_PORT)&&WiFi.status() == WL_CONNECTED) {
  // while (!client.connect(SERVER_IP, SERVER_PORT)) {
    serverConnect++;
    if(serverConnect == 10){
      serverConnect = 0;
      timeServer = 1000;
      resetDisconnect();
      countResetServer++;
      if(countResetServer == 3){
        esp_restart();
      }
      //esp_restart();
    }
    Serial.println("Failed to connect to server, retrying...");
    delay(timeServer);
  }
  Serial.println("Connected to server");
}

void setup() {

  // Attach the pin to the PWM channel
  ledcSetup(pwmChannel1, pwmFrequency, pwmResolution);
  ledcAttachPin(ESC_Pin1, pwmChannel1);

  ledcSetup(pwmChannel2, pwmFrequency, pwmResolution);
  ledcAttachPin(ESC_Pin2, pwmChannel2);

  // set up ESC
  // setupESC();

  // Serial port for debugging purposes
  Serial.begin(115200);

  // Connect to Wi-Fi and server
  connectToWiFi();
  connectToServer();

  //OTA
  OTA.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is ElegantOTA AsyncDemo.");
  });
  ElegantOTA.begin(&OTA);    // Start ElegantOTA
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  OTA.begin();
}

void resetDisconnect(){
  // ledcWrite(0, 0); 
  dutyCycle = getDuty(1000);
  ledcWrite(pwmChannel1, dutyCycle);
  ledcWrite(pwmChannel2, dutyCycle);
  Serial.println("Reset all ESC");
  lastDuty = getDuty(1000);
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
void setPWM(int val) {
  
  int duty;

  if(escInitialize){
    return;
  }

  // Calculate duty based on `val`
  if (val == 0) {
      duty = getDuty(1000);
  } else if (val >= 1 && val <= 9) {
      duty = getDuty(MIN + (val - 1) * (MAX - MIN) / 8);
  } else {
      return; // Invalid `val`, exit function
  }

  if(duty== lastDuty) return;
  Serial.println(val);

  // Smooth transition to the new `duty`
  // int step = (lastDuty < duty) ? 50 : -50;
  // for (int i = lastDuty; (step > 0) ? (i < duty) : (i > duty); i += step) {
  //     ledcWrite(pwmChannel1, i);
  //     ledcWrite(pwmChannel2, i);
  //     Serial.println(i);
  // }

  // Write final `duty` value
  ledcWrite(pwmChannel1, duty);
  ledcWrite(pwmChannel2, duty);
  lastDuty = duty;

  // Debug print final duty
  Serial.println(duty);

  // Uncomment if needed for message handling
  // String mess = create_Message(DEVICE_ID - 1, val);
  // client.println(mess);
}


// get PWM value
uint8_t lastPWM= 0;
uint8_t PWM= 0;
void getPWM(String& message) {
  // int PWM = int(message[message.indexOf('*') - 1]) - '0'; 
  lastPWM = PWM;
  PWM = int(message[message.indexOf('*') - 1]) - '0';
  setPWM(PWM);
}

//heartbeat to check connection
void handleHeartbeat(String& message) {
  // Serial.println("RECEIVE HEARTBEAT");
  int pos = message.indexOf('*');
  int id = int(message[pos - 1]) - 1 - '0';
  if(id == DEVICE_ID-1){
    client.println(message);
  }
}

//handle command
void handleCommand(String& message) {
  String device;
  // Verify the XOR checksum  
  if (verifyXORChecksum(message)) {
    // Serial.println("Checksum valid");
    if (message.startsWith("#HEARTBEAT")) {
      handleHeartbeat(message);
    } 
    else{
      int idPos = message.indexOf(':');
      device = message.substring(1, idPos); 
      if (device == FANID) {
        getPWM(message);
        RESET_TIMEOUT= 30;
      }
    }
  }
  else {
    //Serial.println("Checksum invalid");
  }
}

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
}

void loop() {

  // Initialize ESC
  while (escInitialize) {
    if (!isEscStart) {
      escStartTime = millis(); // Record the start time
      isEscStart = true;
    }

    // first 5s, 1100/20000
    if (millis() - escStartTime <= 5000){
      dutyCycle = getDuty(dutyCycle);
      ledcWrite(pwmChannel1, dutyCycle);
      ledcWrite(pwmChannel2, dutyCycle);
      Serial.println("ESC initialize 1100");
    }

    // After that, 1000/20000
    else if (millis() - escStartTime > 5000) {
      dutyCycle = getDuty(1000); // Reduce duty cycle to 1000
      ledcWrite(pwmChannel1, dutyCycle);
      ledcWrite(pwmChannel2, dutyCycle);
      Serial.println("ESC initialize 1000");
    }

    // Done, exit
    if (millis() - escStartTime >= 10000) {
      escInitialize = false;
    }
  }

  // Reconnect to Wi-Fi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    // resetDisconnect();
    connectToWiFi();
  }
  else {
    wifiConnect = 0;
  }

  // Reconnect to the server if disconnected
  if (WiFi.status() == WL_CONNECTED&&!client.connected()) {
    Serial.println("Server disconnected, reconnecting...");
    // resetDisconnect();
    connectToServer(); 
  }
  else{
    serverConnect = 0;
  }

  // Communication with the server
  while (client.connected()) {
    if (client.available()) {
      String message = client.readStringUntil('\n'); // Read data
      // Serial.println("Received data: " + message);
      handleCommand(message);;
      if(OTAStatus)
        ElegantOTA.loop();
    }
  }

  //reset after 30s not receive data
  if (millis() - lastResetTime >= 1000) {
    lastResetTime = millis();
    RESET_TIMEOUT--;
    // Serial.println(RESET_TIMEOUT); // Debug
    if (RESET_TIMEOUT == 0) {
      ESP.restart();
    }
  }
}