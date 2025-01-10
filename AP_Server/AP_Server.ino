#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiClient.h>
#include <ElegantOTA.h>

//own header
#include "Transceiver.h"
#include "Global.h"
#include "ADC.h"

void setup() {

  // Serial port for debugging 
  Serial.begin(115200);

  // Set up the ESP32 as an access point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password, 1, 0, 16);
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

  //setup variable and client
  setupLed();
  setupGlobalVariable();
  setupClient();
}

// OTA
unsigned long ota_progress_millis = 0;

void onOTAStart() {
  Serial.println("OTA update started!");
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
  // Handle new TCP client connections
  WiFiClient new_client = tcpServer.available();

  if (new_client) {
    bool added = false;
    for (int i = 0; i < NUMBERDEVICE; i++) {
      if (clients[i] == nullptr || !clients[i]->connected()) {
        clients[i] = new WiFiClient(new_client); 

        // reset if 15 connection, max 16
        serverConnected++;
        if(serverConnected == 15){
          ESP.restart();
        }
        Serial.print("New client connected "+ String(serverConnected)+ " : ");
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
    if ((millis() - lastHeartbeatTime[i] > HEARTBEAT_TIMEOUT) || (lastHeartbeatTime[i] == 0)) {
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
      broadcastCommand(createHeartbeatMessage(curHeartbeat));
      curHeartbeat++;
      curHeartbeat = (curHeartbeat > CURRENT_DEVICE+1) ? 1 : curHeartbeat;
    }
  }

  // ADC
  currentMillis = millis();
  if (currentMillis - lastADC >= ADCsample) {
    lastADC = currentMillis;
    for (int i = 0; i < CURRENT_DEVICE; i++) {
      ADC_values[i][countgetADC] = analogRead(adcPins[i]);  // Read ADC values
    }
    countgetADC++;
  }

  if (countgetADC == 10) {
    // Calculate average ADC samples
    for (int i = 0; i < CURRENT_DEVICE; i++) {
      int sum = 0;
      for (int j = 0; j < countgetADC; j++) {
        sum += ADC_values[i][j];
      }
      ADC_samples[i] = sum / 10;
    }

    countsendADC++;
    if (countsendADC == 10) {
      // Get level and reduce noise for each fan
      for (int i = 0; i < CURRENT_DEVICE; i++) {
        int level = updateFanLevel(ADC_samples[i], previousLevels[i], lastADCValues[i]);
        //Serial.println("FAN"+ String(i+1)+" "+level+"    "+String(lastADCValues[i])+"-"+String(ADC_samples[i]));
        // Save last values
        previousLevels[i] = level;
        lastADCValues[i] = ADC_samples[i];

        // Broadcast control message
        String message = createMessage(i, level);
        broadcastCommand(message);
      }
      countsendADC = 0;
    }

    // Reset for new cycle
    countgetADC = 0;
    for (int i = 0; i < CURRENT_DEVICE; i++) {
      ADC_samples[i] = 0;
    }
  }

  //reset after 30s not receive data
  // if (millis() - lastResetTime >= 1000) {
  //   lastResetTime = millis();
  //   RESET_TIMEOUT--;
  //   Serial.println(RESET_TIMEOUT); // Debug
  //   if (RESET_TIMEOUT == 0) {
  //     ESP.restart();
  //   }
  // }
}