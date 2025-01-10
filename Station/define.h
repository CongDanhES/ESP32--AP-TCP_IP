#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

// Define DEVICE_ID
#define DEVICE_ID 1
// Device-specific configurations
#if DEVICE_ID == 1
    #define FANID         "FAN1"
    #define MDNS_NAME     "QUAT1"
    #define PWM_PIN       2 
    #define HEARTBEAT     "HEARTBEAT1"
    #define LOCAL_IP      IPAddress(192, 168, 4, 165)
#elif DEVICE_ID == 2
    #define FANID         "FAN2"
    #define MDNS_NAME     "QUAT2"
    #define PWM_PIN       2 
    #define HEARTBEAT     "HEARTBEAT2"
    #define LOCAL_IP      IPAddress(192, 168, 4, 166)
#elif DEVICE_ID == 3
    #define FANID         "FAN3"
    #define MDNS_NAME     "QUAT3"
    #define PWM_PIN       2 
    #define HEARTBEAT     "HEARTBEAT3"
    #define LOCAL_IP      IPAddress(192, 168, 4, 167)
#elif DEVICE_ID == 4
    #define FANID         "FAN4"
    #define MDNS_NAME     "QUAT4"
    #define PWM_PIN       2 
    #define HEARTBEAT     "HEARTBEAT4"
    #define LOCAL_IP      IPAddress(192, 168, 4, 168)
#elif DEVICE_ID == 5
    #define FANID         "FAN5"
    #define MDNS_NAME     "QUAT5"
    #define PWM_PIN       2 
    #define HEARTBEAT     "HEARTBEAT5"
    #define LOCAL_IP      IPAddress(192, 168, 4, 169)
#elif DEVICE_ID == 6
    #define FANID         "FAN6"
    #define MDNS_NAME     "QUAT6"
    #define PWM_PIN       2 
    #define HEARTBEAT     "HEARTBEAT6"
    #define LOCAL_IP      IPAddress(192, 168, 4, 170)
#else
    #error "Invalid DEVICE_ID! Please define a valid DEVICE_ID between 1 and 6."
#endif

// Common configurations
#define MAX           1600
#define MIN           1200
#define GATEWAY       IPAddress(192, 168, 4, 1)
#define SUBNET        IPAddress(255, 255, 255, 0)
#define SSID          "TTD_Quat"
#define PASSWORD      "TTD.2022"
#define SERVER_IP     "192.168.4.115"
#define SERVER_PORT   5000

#define ESC_Pin1       18
#define ESC_Pin2       19

#endif // DEVICE_CONFIG_H