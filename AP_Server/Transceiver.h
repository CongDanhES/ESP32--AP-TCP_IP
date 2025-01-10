#ifndef TRANSCEIVER_H
#define TRANSCEIVER_H

#include "Global.h"

// Create message
String createMessage(int fanIndex, int value) {
  String id = "FAN" + String(fanIndex + 1);
  String val = String(value);
  String data = id + ":" + val;
  uint8_t checksum = 0;
  for (size_t i = 0; i < data.length(); i++) {
    checksum ^= data[i];
  }
  char checksumStr[3];
  sprintf(checksumStr, "%02X", checksum);
  String message = "#" + data + "*" + checksumStr + "%";
  return message;
}

// Function to create heartbeat message
String createHeartbeatMessage(int i) {
  String message = "HEARTBEAT" + String(i);
  uint8_t checksum = 0;
  for (size_t i = 0; i < message.length(); i++) {
    checksum ^= message[i];
  }
  char checksumStr[3];
  sprintf(checksumStr, "%02X", checksum);
  message = "#" + message + "*" + String(checksumStr) + "%";
  return message;
}

// Broadcast message
void broadcastCommand(String message) {
  for (int i = 0; i < NUMBERDEVICE; i++) {
    if (clients[i] != nullptr && clients[i]->connected()) {
      clients[i]->println(message); // Send to client
    }
  }
}

// Checksum 
bool verifyXORChecksum(const String& message) {
  // Find the position of the '*' character
  int starPos = message.indexOf('*');
  if (starPos == -1) {
    return false;
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

// Heartbeat
void handleHeartbeat(String& message) {
  int pos = message.indexOf('*');
  int id = int(message[pos - 1]) - 1 - '0';
  lastHeartbeatTime[id] = millis();
  connectStatus[id] = true;
}

// Update fan value from station 
void updateFanValue(String& message) {
  int idPos = message.indexOf(':');
  String device = message.substring(1, idPos);
  int val = int(message[message.indexOf('*') - 1]) - 48;

  // Parse fan index from the device name
  if (device.startsWith("FAN")) {
    int fanIndex = device.substring(3).toInt() - 1; // Extract fan number and convert to 0-based index
    fanValues[fanIndex] = val;
    Serial.println("Update: FAN" + String(fanIndex + 1) + ":" + String(val));
  }
}

// Handle data
void handleCommand(String& message) {
  RESET_TIMEOUT = 30;
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

// update led status for check connection
void updateLedStatus(int led, int status) {
  // Validate the input (ensure led is between 1 and 4)
  if (led >= 1 && led <= CURRENT_DEVICE) {
    // Map led index (1-based) to array index (0-based)
    int pin = ledPins[led - 1];
    digitalWrite(pin, status == 1 ? HIGH : LOW);
  }
}

#endif

