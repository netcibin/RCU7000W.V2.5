// Import required libraries
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <string.h>
#include <HardwareSerial.h>
#include <MCP23017.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <DHT.h>
#include "esp_task_wdt.h"
#include "time.h"
#include <Update.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <FS.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

String clientId;

// Device information
#define DEVICE_MODEL "RCU7000W"
String FW_Version = "1.0.0";

// Update URLs (to be received from MQTT)
String firmwareURL;
String filesystemURL;
bool firmwareUpdateRequired = false;
bool filesystemUpdateRequired = false;
bool mandatoryUpdate = false;
// Flags to track update status
bool firmwareUpdated = false;
bool filesystemUpdated = false;

// MQTT Topics
const char* publishTopic = "rcu/update";   // RCU will publish data here
const char* subscribeTopic = "rcu/control"; // RCU will subscribe for commands


// NTP server and offsets
const char* ntpServer = "p1.mysite.net"; // NTP server IP or pool.ntp.org
const long gmtOffset_sec = 14400;      // 4 x 3600 = 14400 seconds. Adjust as per your time zone
const int daylightOffset_sec = 0;    // Adjust for daylight saving time

bool timeSynced = false; // Flag to check if time was ever synced

// to work with switch statement instead of using if..else statement
#define CMD_AC_POWER 501
#define CMD_FAN_MODE 502
#define CMD_SET_POINT 503
#define CMD_PRE_SENSOR 504
#define CMD_SW1 601 //Pendant
#define CMD_SW2 602 //Bed Lamp
#define CMD_SW3 603 //Master
#define CMD_SW4 604 //Reading
#define CMD_SW5 605 //DND
#define CMD_SW6 606 //MUR

char buffer[100];
int buffer_pos = 0;

char buffer1[128]; // Buffer for Serial1
int buffer1_pos = 0;

#define WiFiRST_BUTTON 0 // Change to any available GPIO pin
#define WiFiRST_HOLD_TIME  5000  // Hold 5 seconds to reset the wifi settings

const int keyTag = 27;
const int pirPin = 25;     // Presence Sensor

#define Input6 36 // Master Button Living Room
#define Input7 39 // Pendant Button Living Room
#define Input8 34 // DND Button Living Room
#define Input9 35 // Balcony Button Bedroom

// mmWaveRadar pins config
HardwareSerial SerialPort(2); // use UART2 for thermostat communication

#define SERIAL_RX_PIN 16// 32 for UART2
#define SERIAL_TX_PIN 17// 33 for UART2

unsigned long lastCleanupTime = 0;
const unsigned long cleanupInterval = 30000; // 30 seconds
const unsigned long pingInterval = 10000; // 10 seconds

String presenceStatus; // Variable to store presence status (On/Off)
int pres_sensor_2;
volatile int keyTagState = 0;
String keyStatus;
String update_key;
unsigned long previousLEDMillis = 0;
const long LEDinterval = 600;  // Update interval in milliseconds

bool lastbellButton = LOW;
unsigned long bellStartTime = 0;  // Tracks when the relay was turned on
bool bellActive = false;          // Tracks whether the bell is currently active
unsigned long lastDebounceTime = 0;       // For debouncing
const unsigned long debounceDelay = 50;  // Debounce interval in ms

bool callStopExecuted = false; // Global or static flag to track execution

int masterState = 0;

String wifiStatus;
String ipAddress;
String wifiSignal;

String mqttstatus = "Not Connected";

bool isScanning = false; // Global flag to track scanning state

bool fanModeOverride = false;
int ac_power = 0;
int default_set_point = 24; // Default value in case the file is missing or unreadable
int init_set_point = 23; // When key is inserted in the keyTag
bool setPointInitialized = false; // Global flag variable
const char* setPointFilePath = "/settings/set_point.txt";
int set_point;
int current_temperature;
int current_humidity;
int fan_mode;
int dnd_status = 0;
int mur_status = 0;
int balcony_status = 0;

static bool key_send = false;

String _ac_status;
String _fan_mode;
String _dnd_status;
String _mur_status;
String _mar_status;

unsigned long previousMillis = 0;
const long interval = 5000; // Update interval in milliseconds (2 seconds)
unsigned long startTime; // For the below line
const unsigned long startDelay = 5000; // For AC Switch ON automatically after the board power cycle.
bool fanInitilized = false;

const unsigned long checkwifiInterval = 10000; // 10 seconds
unsigned long previouswifiMillis = 0;

#define DHTPIN 26     // Digital pin connected to the DHT sensor

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);


bool rlyState[13] = {false}; //boolean array of 13 elements, all initialized to false
#define MCP23017_I2C_ADDRESS 0x20  // I2C address of the MCP23017a IC
const uint8_t IN10 = 0;      // GPA0 (21) of the MCP23017 (Input Pin)
const uint8_t IN11 = 1;      // GPA1 (22) of the MCP23017 (Input Pin)
const uint8_t RLY9 = 2;      // GPA2 (23) of the MCP23017
const uint8_t RLY10 = 3;      // GPA3 (24) of the MCP23017
const uint8_t RLY11 = 4;      // GPA4 (25) of the MCP23017
const uint8_t RLY12 = 5;      // GPA5 (26) of the MCP23017
const uint8_t RLY13 = 6;      // GPA6 (27) of the MCP23017
const uint8_t RLY14 = 7;      // GPA7 (28) of the MCP23017 - Output only pin
const uint8_t RLY1 = 8;   // GPB0 (1) of the MCP23017
const uint8_t RLY2 = 9;   // GPB1 (2) of the MCP23017
const uint8_t RLY3 = 10;  // GPB2 (3) of the MCP23017
const uint8_t RLY4 = 11;  // GPB3 (4) of the MCP23017
const uint8_t RLY5 = 12;  // GPB4 (5) of the MCP23017
const uint8_t RLY6 = 13;  // GPB5 (6) of the MCP23017
const uint8_t RLY7 = 14;  // GPB6 (7) of the MCP23017
const uint8_t RLY8 = 15;  // GPB7 (8) of the MCP23017 - Output only pin
MCP23017 mcp23017a = MCP23017(MCP23017_I2C_ADDRESS);  // instance of the connected MCP23017a IC

const int rlyPins[] = {RLY1,RLY2,RLY3,RLY4,RLY5,RLY6,RLY7,RLY8,RLY9,RLY10,RLY11,RLY12,RLY13,RLY14};

#define MCP23017_2_I2C_ADDRESS 0x21  // I2C address of the MCP23017b IC
const uint8_t LED10 = 0;     // GPA0 (21) of the MCP23017 (Input Pin) IN8 changed to output mode (Pendant Indication Living Room Switch Panel)
const uint8_t LED11 = 1;     // GPA1 (22) of the MCP23017 (Input Pin) IN9 changed to output mode (Balcony Indication Bed Room Writing Table Switch Panel)
const uint8_t LED2 = 2;      // GPA2 (23) of the MCP23017 (Panel Led Output) DND Light
const uint8_t LED3 = 3;      // GPA3 (24) of the MCP23017 (Panel Led Output) MUR Light
const uint8_t LED4 = 4;      // GPA4 (25) of the MCP23017 (Panel Led Output)
const uint8_t LED5 = 5;      // GPA5 (26) of the MCP23017 (Panel Led Output)
const uint8_t LED6 = 6;      // GPA6 (27) of the MCP23017 (Panel Led Output)
const uint8_t LED7 = 7;      // GPA7 (28) of the MCP23017 - Output only pin (Panel Led Output)
const uint8_t IN1 = 8;   // GPB0 (1) of the MCP23017 (Input Pin)
const uint8_t IN2 = 9;   // GPB1 (2) of the MCP23017 (Input Pin)
const uint8_t IN3 = 10;  // GPB2 (3) of the MCP23017 (Input Pin)
const uint8_t IN4 = 11;  // GPB3 (4) of the MCP23017 (Input Pin)
const uint8_t IN5 = 12;  // GPB4 (5) of the MCP23017 (Input Pin)
const uint8_t LED8 = 13;  // GPB5 (6) of the MCP23017 (Input Pin) IN6 changed to output mode (Master Indication Living Room Switch Panel)
const uint8_t LED9 = 14;  // GPB6 (7) of the MCP23017 (Input Pin) IN7 changed to output mode (DND Indication Living Room Switch Panel)
const uint8_t LED1 = 15;  // GPB7 (8) of the MCP23017 - Output only pin (Panel Led Output) Bell Button Light


MCP23017 mcp23017b = MCP23017(MCP23017_2_I2C_ADDRESS);  // instance of the connected MCP23017b IC
const int inputPins[] = {IN1,IN2,IN3,IN4,IN5};
const int panelLedPins[] = {LED1,LED2,LED3,LED4,LED5,LED6,LED7,LED8,LED9,LED10,LED11};

bool lastButtonStateIN1 = false;
bool lastButtonStateIN2 = false;
bool lastButtonStateIN3 = false;
bool lastButtonStateIN4 = false;
bool lastButtonStateIN5 = false;
bool lastButtonStateIN6 = false; // Input6
bool lastButtonStateIN7 = false; // Input7
bool lastButtonStateIN8 = false; // Input8
bool lastButtonStateIN9 = false; // Input9

Preferences prefmqtt;

unsigned long lastMsg = 0;
const long interval_mqtt = 15000; // 10 seconds interval

static unsigned long lastMQTTReconnectAttempt = 0;

// Structure to hold MQTT settings
struct MQTTConfig {
    String server;
    int port;
    String username;
    String password;
    bool isValid;
};

MQTTConfig mqttConfig;


void mqttSub_cb(char* topic, byte* payload, unsigned int length) {
    Serial.print("📩 Message received on topic: ");
    Serial.println(topic);

    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("📨 Payload: ");
    Serial.println(message);

    // ✅ Handle different commands based on message

    // Check if message starts with "client-id:MAC-ADDRESS-"
        if (message.startsWith(clientId)) {
            String command = message.substring(clientId.length()); // Extract command

            // Process Command
            if (command == "Reboot") {
                Serial.println("🔄 Reboot command received!");
                ESP.restart();
            } 
            //  handleOTAUpdate();  // Call OTA function
    }
}

// ✅ Declare function before calling it
MQTTConfig loadMQTTSettings();

// Function to connect to MQTT broker
bool connectMQTT(MQTTConfig config);

// Function to connect to MQTT broker
bool connectMQTT(MQTTConfig config) {
    if (!config.isValid) {
        Serial.println("⚠️ No valid MQTT settings found in NVS!");
        return false;
    }
/*
    // Debugging
    Serial.println("MQTT Server: " + mqttConfig.server);
    Serial.println("MQTT Port: " + String(mqttConfig.port));
    Serial.println("MQTT User: " + mqttConfig.username);
    Serial.println("MQTT Pass: " + mqttConfig.password);
*/

    Serial.println("⏳ Connecting to MQTT...");
    mqttClient.setServer(config.server.c_str(), config.port);

    //Set the callback function here
    mqttClient.setCallback(mqttSub_cb);

   // String clientId = "Client-" + String(WiFi.macAddress());
    clientId = "Client-" + String(WiFi.macAddress());

    if (mqttClient.connect(clientId.c_str(), config.username.c_str(), config.password.c_str())) {
        Serial.println("✅ Connected to MQTT broker: " + config.server);
        mqttClient.subscribe(subscribeTopic); // Subscribe to topic
        mqttstatus = "Connected";
        return true;
    } else {
        Serial.println("❌ MQTT Connection Failed! Error: " + String(mqttClient.state()));
        mqttstatus = "Disconnected";
        return false;
    }
}

// Function to load MQTT settings from NVS
MQTTConfig loadMQTTSettings() {
    MQTTConfig config;
    prefmqtt.begin("mqtt", true);
    config.server = prefmqtt.getString("server", "");
    config.port = prefmqtt.getInt("port", 0);
    config.username = prefmqtt.getString("user", "");
    config.password = prefmqtt.getString("pass", "");
    prefmqtt.end();

    config.isValid = !config.server.isEmpty() && config.port != 0;
    return config;
}

// Function to save MQTT settings from WebSocket data
void saveMQTTSettings(String server, int port, String user, String pass) {
    prefmqtt.begin("mqtt", false);
    prefmqtt.putString("server", server);
    prefmqtt.putInt("port", port);
    prefmqtt.putString("user", user);
    prefmqtt.putString("pass", pass);
    prefmqtt.end();
    Serial.println("✅ MQTT settings saved successfully.");
}

void mqtt_cb(){
  if (!mqttClient.connected()) {  // Check if MQTT is disconnected
            if (!mqttConfig.isValid) { 
                mqttConfig = loadMQTTSettings();  // Load settings from NVS only once
            }

            // Only attempt reconnect if settings exist
            if (mqttConfig.isValid) { 
                unsigned long mqtt_now = millis();
                if (mqtt_now - lastMQTTReconnectAttempt > 10000) {  // Attempt to reconnect every 10s
                    lastMQTTReconnectAttempt = mqtt_now;
                    connectMQTT(mqttConfig);
                }
            }
        } else {
            mqttClient.loop();  // Maintain MQTT connection

            // ✅ Only publish if both WiFi and MQTT are connected
            unsigned long now_mqtt = millis();
            if (now_mqtt - lastMsg > interval_mqtt) {
                lastMsg = now_mqtt;
                mqttPublish();
            }
        }
}

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    char *str = (char*)data;
    if (strncmp(str, "toggle:", 7) == 0) {
      int index = atoi(&str[7]) - 1;
      rlyState[index] = !rlyState[index];
      mcp23017a.digitalWrite(rlyPins[index], rlyState[index] ? HIGH : LOW);
      // LED Logic function to update the LED status on button
      updateLEDStatus(index);
      // LED logic function end.

      String stateMsg = "toggle:" + String(index + 1) + ":" + String(rlyState[index] ? 1 : 0);
      ws.textAll(stateMsg);
    }

  else if (strncmp(str, "keytagStatus:", 13) == 0) {
      int newState = atoi(&str[13]);
      if (newState == 0 || newState == 1) {
        noInterrupts(); // Disable interrupts
        keyTagState = newState; // Safely update keytag status
        interrupts(); // Re-enable interrupts
        if (keyTagState == 1){
          keyStatus = "IN";
        }
        else{
          keyStatus = "OUT";
        }
        send_keystatus();
        update_key = "Manual";
        String key_status = "{\"key\": \"" + keyStatus + "\"}";
        String KeyTag_Status = "{\"updatekey\": \"" + update_key + "\"}";
        ws.textAll(key_status); // Send updated state to all WebSocket clients
        ws.textAll(KeyTag_Status); // Send updated state to all WebSocket clients
      }
    }
    else if (strncmp(str, "restart", 7) == 0) {
        delay(10000);  // Wait for 5 seconds
        esp_task_wdt_reset();  // Reset the watchdog timer to prevent it from triggering
        Serial.println("Restarting RCU...");
        ESP.restart();         // Restart the ESP32
    }
    else if (strncmp(str, "setpoint:", 9) == 0) {
        set_point = atoi(&str[9]); // Converts the number after "setpoint:" into an integer and save the value to set_point
        send_setpoint(set_point);
        String set_tem = "{\"setpoint\": \"" + String(set_point) + "\"}";
        ws.textAll(set_tem); // Send updated set_point value to all WebSocket clients.
    }
    else if (strncmp(str, "def_setpoint:", 13) == 0) {
        int def_set_point = atoi(&str[13]); // Converts the number after "def_setpoint:" into an integer and save the value to def_set_point
        writeSetPointToFile(setPointFilePath, def_set_point);
    }
    else if (strncmp(str, "masterstatus:", 13) == 0) {
      int masnewState = atoi(&str[13]);
      if (masnewState == 0 || masnewState == 1){
        masterState = masnewState;
        if (masterState == 0){
            _mar_status = "OFF";
          } else if (masterState == 1){
            _mar_status = "ON";
          }
          else {
            _mar_status = "Status Error";
          }
          char Message[10];
          snprintf(Message, sizeof(Message), "SET_SW3:%d", masterState ? 1 : 0);
          sendData(Message);  // Send the status message over serial
          String mar = "{\"MASStatus\": \"" + _mar_status + "\"}";
          ws.textAll(mar);
      }
    }
    else if (strncmp(str, "dndstatus:", 10) == 0) {
        int dndnewState = atoi(&str[10]);
      if (dndnewState == 0 || dndnewState == 1){
        dnd_status = dndnewState;
        if (dnd_status == 0){
            _dnd_status = "OFF";
          }
          else if (dnd_status == 1){
            _dnd_status = "ON";
          }
          else {
            _dnd_status = "Status Error";
          }
          mcp23017b.digitalWrite(LED2, dnd_status ? HIGH : LOW); // Switch on/off DND Led
          mcp23017b.digitalWrite(LED9, dnd_status ? HIGH : LOW); // Switch on/off DND Led
          char Message[10];
          snprintf(Message, sizeof(Message), "SET_SW5:%d", dnd_status ? 1 : 0);
          sendData(Message);  // Send the status message over serial
          String dnd = "{\"DNDStatus\": \"" + _dnd_status + "\"}";
          ws.textAll(dnd);
      }
    }
    else if (strncmp(str, "murstatus:", 10) == 0) {
        int murnewState = atoi(&str[10]);
        if (murnewState == 0 || murnewState == 1){
        mur_status = murnewState;
        if (mur_status == 0){
            _mur_status = "OFF";
          } else if (mur_status == 1){
            _mur_status = "ON";
          }
          else {
            _mur_status = "Status Error";
          }
          mcp23017b.digitalWrite(LED3, mur_status ? HIGH : LOW); // Switch on/off MUR Led
          char Message[10];
          snprintf(Message, sizeof(Message), "SET_SW6:%d", mur_status ? 1 : 0);
          sendData(Message);  // Send the status message over serial
          String mur = "{\"MURStatus\": \"" + _mur_status + "\"}";   
          ws.textAll(mur);
        }
      }
  // Check if the command is MQTT
    else if (strncmp(str, "MQTT:", 5) == 0) {  
        Serial.println("🟢 MQTT configuration command received!");

        String payload = String(str + 5);  // Extract JSON content after "MQTT:"
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.println("❌ Failed to parse MQTT JSON");
            return;
        }

        // Extract MQTT settings from JSON
        String server = doc["server"];
        int port = doc["port"];
        String user = doc["user"];
        String password = doc["pass"];
        String topic = doc["topic"];

        if (server.isEmpty() || port == 0) {
            Serial.println("⚠️ Invalid MQTT settings. Skipping...");
            return;
        }

        // Store settings in NVS
        saveMQTTSettings(server, port, user, password);
        Serial.println("✅ MQTT settings saved!");

       /* 
       // Attempt to reconnect with new settings
          MQTTConfig mqttConfig = loadMQTTSettings();
        if (mqttConfig.isValid) {
            connectMQTT(mqttConfig);
        } 
        */
      }
      else {
            Serial.println("⚠️ Unknown WebSocket command!");
        }
    }
}

void eventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
      Serial.printf("Client #%u responded to ping.\n", client->id());
      break;
    case WS_EVT_ERROR:
      Serial.printf("WebSocket error on client #%u\n", client->id());
      break;
  }
}

String processor(const String& var) {
    for (int i = 0; i < 14; i++) {
        if (var == "CHECK" + String(i + 1)) {
            return rlyState[i] ? "checked" : "";
        } else if (var == "STATE" + String(i + 1)) {
            return rlyState[i] ? "ON" : "OFF";
        }
    }
    return String();
}

void IRAM_ATTR handleInterrupt() {
  // Read the current state of the pin and save it to the variable
 keyTagState = digitalRead(keyTag);
  if (keyTagState == HIGH) {
    keyStatus = "IN";
  } else {
    keyStatus = "OUT";
  }
    update_key = "Auto";
}

void updateFanState() {
    mcp23017a.digitalWrite(rlyPins[11], rlyState[11] ? HIGH : LOW);
    mcp23017a.digitalWrite(rlyPins[12], rlyState[12] ? HIGH : LOW);

    // Send WebSocket message
    String stateMsg1 = "toggle:12:" + String(rlyState[11] ? 1 : 0);
    String stateMsg2 = "toggle:13:" + String(rlyState[12] ? 1 : 0);
    ws.textAll(stateMsg1);
    ws.textAll(stateMsg2);
}

// Function to get command type
int getCommandType(const char* command) {
    if (strncmp(command, "AC_POWER:", 9) == 0) return CMD_AC_POWER;
    if (strncmp(command, "FAN_MODE:", 9) == 0) return CMD_FAN_MODE;
    if (strncmp(command, "SET_POINT:", 10) == 0) return CMD_SET_POINT;
    if (strncmp(command, "PRESENCE:", 9) == 0) return CMD_PRE_SENSOR;
    if (strncmp(command, "SW1:", 4) == 0) return CMD_SW1; //Pendant
    if (strncmp(command, "SW2:", 4) == 0) return CMD_SW2; //Bed
    if (strncmp(command, "SW3:", 4) == 0) return CMD_SW3; //Master
    if (strncmp(command, "SW4:", 4) == 0) return CMD_SW4; //Reading Light
    if (strncmp(command, "SW5:", 4) == 0) return CMD_SW5; //DND
    if (strncmp(command, "SW6:", 4) == 0) return CMD_SW6; //MUR
    // Add more commands as needed
    return -1; // Unknown command
}

// Function to parse and handle received data
void parseData(char* data) {
    int cmdType = getCommandType(data);
    switch (cmdType) {
        case CMD_AC_POWER: {
            // int ac_power = atoi(data + 9);
            ac_power = atoi(data + 9);
            Serial.print("AC Power:");
            Serial.println(ac_power);
            // Handle ac_power value
            // Toggle the relay state and send WebSocket message
            if (ac_power == 0) {
                rlyState[11] = 0;
                rlyState[12] = 0;
              }
            else if (ac_power == 1) {
              if (fan_mode == 1) {
                  rlyState[11] = 1;
                  rlyState[12] = 0;
                } 
                else if (fan_mode == 2) {
                  rlyState[11] = 1;
                  rlyState[12] = 1;
                } 
                else if (fan_mode == 3) {
                  rlyState[11] = 0;
                  rlyState[12] = 1;
                }
            }
            updateFanState();
            break;
        }
        case CMD_FAN_MODE: {
            fan_mode = atoi(data + 9);
            Serial.print("FAN MODE:");
            Serial.println(fan_mode);
            if (ac_power == 1) { // Only update relays if AC power is ON
                if (fan_mode == 1) {
                    rlyState[11] = 1;
                    rlyState[12] = 0;
                } else if (fan_mode == 2) {
                    rlyState[11] = 1;
                    rlyState[12] = 1;
                } else if (fan_mode == 3) {
                    rlyState[11] = 0;
                    rlyState[12] = 1;
                }
                updateFanState();
              }
            break;
        }
        case CMD_SET_POINT: {
            set_point = atoi(data + 10);
            Serial.print("Set Point:");
            Serial.println(set_point);
            break;
        }
         case CMD_PRE_SENSOR: {
            pres_sensor_2 = atoi(data + 9);
            Serial.print("Presence2:");
            Serial.println(pres_sensor_2);
            break;
        }
        case CMD_SW1: {
            int sw1 = atoi(data + 4);
            Serial.print("SW1:");
            Serial.println(sw1);
            rlyState[5] = sw1;
            mcp23017a.digitalWrite(rlyPins[5], rlyState[5] ? HIGH : LOW);
            String stateMsg = "toggle:6:" + String(rlyState[5] ? 1 : 0);
            ws.textAll(stateMsg);
            break;
        }
        case CMD_SW2: {
            int sw2 = atoi(data + 4);
            Serial.print("SW2:");
            Serial.println(sw2);
            rlyState[6] = sw2;
            mcp23017a.digitalWrite(rlyPins[6], rlyState[6] ? HIGH : LOW);
            String stateMsg = "toggle:7:" + String(rlyState[6] ? 1 : 0);
            ws.textAll(stateMsg);
            break;
        }
        case CMD_SW3: {
            int sw3 = atoi(data + 4);
            Serial.print("SW3:");
            Serial.println(sw3);
            masterState = sw3;
            if (masterState == 0){
            _mar_status = "OFF";
            } else if (masterState == 1){
              _mar_status = "ON";
            }
            else {
              _mar_status = "Status Error";
            }
            String mar = "{\"MASStatus\": \"" + _mar_status + "\"}";
            ws.textAll(mar);
            mcp23017b.digitalWrite(LED8, masterState ? HIGH : LOW);
            break;
        }
        case CMD_SW4: {
            int sw4 = atoi(data + 4);
            Serial.print("SW4:");
            Serial.println(sw4);
            rlyState[7] = sw4;
            mcp23017a.digitalWrite(rlyPins[7], rlyState[7] ? HIGH : LOW);
            String stateMsg = "toggle:8:" + String(rlyState[7] ? 1 : 0);
            ws.textAll(stateMsg);
            break;
        }
        case CMD_SW5: {
            int sw5 = atoi(data + 4);
            Serial.print("SW5:");
            Serial.println(sw5);
            // Handle SW5 value
            dnd_status = sw5;
            if (dnd_status == 0){
            _dnd_status = "OFF";
            }
            else if (dnd_status == 1){
              _dnd_status = "ON";
            }
            else {
              _dnd_status = "Status Error";
            }
            String dnd = "{\"DNDStatus\": \"" + _dnd_status + "\"}";
            ws.textAll(dnd);
            mcp23017b.digitalWrite(LED2, dnd_status ? HIGH : LOW);
            mcp23017b.digitalWrite(LED9, dnd_status ? HIGH : LOW);
            break;
        }
        case CMD_SW6: {
            int sw6 = atoi(data + 4);
            Serial.print("SW6:");
            Serial.println(sw6);
            mur_status = sw6;
            if (mur_status == 0){
            _mur_status = "OFF";
            } else if (mur_status == 1){
              _mur_status = "ON";
            }
            else {
              _mur_status = "Status Error";
            }
            String mur = "{\"MURStatus\": \"" + _mur_status + "\"}";
            ws.textAll(mur);
            mcp23017b.digitalWrite(LED3, dnd_status ? HIGH : LOW);
            break;
        }
        
        // Add more cases as needed
        default: {
            Serial.println("Unknown command");
            break;
        }
    }
}

void serialEvent() {
    while (SerialPort.available()) {
        char inChar = (char)SerialPort.read();
        if (inChar == '\n') {
            buffer[buffer_pos] = '\0'; // Null-terminate the string
            if (keyTagState) {          // Only process when keyTagState is true
                parseData(buffer);
            }
            buffer_pos = 0; // Reset the buffer position
        } else {
            buffer[buffer_pos++] = inChar;
        }
    }
}

// Function to send data over serial
void sendData(const char* data) {
    SerialPort.println(data); // Send data with a newline character
    delay(100); // A short delay between sends to avoid overwhelming the receiver
}

// Function to read the set point from a file and return it as an integer
int readSetPointFromFile(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.println("Failed to open file for reading. Using default set_point value.");
        return default_set_point;  // Use the existing default value
    }

    String valueStr = file.readStringUntil('\n');  // Read file content as a string
    file.close();
    
    int value = valueStr.toInt();  // Convert the string to an integer
    if (value == 0 && valueStr != "0") {  // Check for failed conversion
        Serial.println("Invalid value in file. Using default set_point value.");
        return default_set_point; //function exit if it returns default_set_point. Or it will continue to the next return value outside if statement. 
    }

    return value;
}

// Function to write the set point to a file
void writeSetPointToFile(const char* path, int def_setPoint) {
    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.println("Failed to open file for writing.");
        return;
    }
    
    file.println(def_setPoint);  // Write the set point as a string to the file
    file.close();
    Serial.println("Default Set point saved to file.");
    default_set_point = def_setPoint; // update the existing global value to change the active value.
}

void connectTimeServer() {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Time synchronization initialized.");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) { // Wait max 5 sec
      timeSynced = true; // Mark time as synced
      Serial.println("Time synchronized successfully.");
    } else {
      Serial.println("Failed to sync time.");
    }
}

void timeSync() {
    static unsigned long lastSyncAttempt = 0;
    const unsigned long syncRetryInterval = 60000; // Retry NTP sync every 60 seconds if needed

    if (!timeSynced) { // Only attempt resync if time is not set
      unsigned long now = millis();
      if (now - lastSyncAttempt >= syncRetryInterval) {
        Serial.println("Retrying NTP sync...");
        connectTimeServer();
        lastSyncAttempt = now;
      }
    }
}

void sendTimeToDisplay() {
    static int lastMinute = -1; // Keep track of last sent minute
    struct tm timeinfo;

    if (getLocalTime(&timeinfo)) {
      if (timeinfo.tm_min != lastMinute) { // Only send once per minute
        lastMinute = timeinfo.tm_min;

        char timeString[6]; // "HH:MM"
        strftime(timeString, sizeof(timeString), "%H:%M", &timeinfo);

        String command = "SET_TIME:";
        command += timeString;

        SerialPort.println(command);
        Serial.println(command); // Debug output
      }
    } else {
      Serial.println("Time retrieval failed.");
    }
}

void configureMCPaPinMode() {
    // Configure output pins
    for (int i = 0; i < 14; i++) {
          mcp23017a.pinMode(rlyPins[i], OUTPUT);
      }
        // Initialize rlyState array with the actual state of the relays
    for (int i = 0; i < 13; i++) {
        rlyState[i] = (mcp23017a.digitalRead(rlyPins[i]) == HIGH);
    }
   // Configure Input pins
   mcp23017a.pinMode(IN10, INPUT_PULLUP);
   mcp23017a.pinMode(IN11, INPUT_PULLUP);
}

void configureMCPbPinMode(){
  // Configure output pins
   for (int i = 0; i < 5; i++) {
    // Third argument inverts the polarity of the input value when read
        mcp23017b.pinMode(inputPins[i], INPUT_PULLUP, true);
    }
   // mcp23017b.pinMode(IN9, INPUT, true);
    for (int i = 0; i < 11; i++) {
        mcp23017b.pinMode(panelLedPins[i], OUTPUT);
    }
}

void switchPanel(){

      if (!keyTagState) return;
      // Read and write individual inputs and outputs 
      //  mcp23017b.digitalWrite(LED1, mcp23017b.digitalRead(IN1));
      // Read the current state of the button
      bool bellButton = mcp23017b.digitalRead(IN1);
      if (millis() - lastDebounceTime > debounceDelay) {
            if (bellButton == HIGH && keyTagState == HIGH && !bellActive){
                  if (dnd_status == 0 && lastbellButton == LOW){
                  mcp23017a.digitalWrite(rlyPins[0], HIGH);
                  bellStartTime = millis();                  // Record the start time
                  bellActive = true;                         // Set the bell active state
                  lastDebounceTime = millis();            // Update debounce time
                  }
              }
          }

      // If the bell is active and 500 ms has passed, turn off the relay
      if (bellActive && millis() - bellStartTime >= 400) {
        mcp23017a.digitalWrite(rlyPins[0], LOW);   // Turn off relay
        bellActive = false;                        // Reset the bell active state
      }
    lastbellButton = bellButton;
    
    bool currentButtonStateIN2 = mcp23017b.digitalRead(IN2); // Bath Light
    bool currentButtonStateIN3 = mcp23017b.digitalRead(IN3); // Bath Mirror
    bool currentButtonStateIN4 = mcp23017b.digitalRead(IN4); // Bed Lamp
    bool currentButtonStateIN5 = mcp23017b.digitalRead(IN5); // Reading Lamp
    
    bool currentButtonStateIN6 = digitalRead(Input6); // Living room Master button
    bool currentButtonStateIN7 = digitalRead(Input7); // Living room DND
    bool currentButtonStateIN8 = digitalRead(Input8); // Living room Pendant
    bool currentButtonStateIN9 = digitalRead(Input9); // Bed room Balcony light

    if (currentButtonStateIN2 && !lastButtonStateIN2) {
        // Toggle the relay state and send WebSocket message
        rlyState[1] = !rlyState[1];
        mcp23017a.digitalWrite(rlyPins[1], rlyState[1] ? HIGH : LOW);
        updateLEDStatus(1);
        String stateMsg = "toggle:2:" + String(rlyState[1] ? 1 : 0);
        ws.textAll(stateMsg);
    }

    if (currentButtonStateIN3 && !lastButtonStateIN3) {
        // Toggle the relay state and send WebSocket message
        rlyState[2] = !rlyState[2];
        mcp23017a.digitalWrite(rlyPins[2], rlyState[2] ? HIGH : LOW);
        updateLEDStatus(2);
        String stateMsg = "toggle:3:" + String(rlyState[2] ? 1 : 0);
        ws.textAll(stateMsg);
    }

    if (currentButtonStateIN4 && !lastButtonStateIN4) {
        // Toggle the relay state and send WebSocket message
        rlyState[3] = !rlyState[3];
        mcp23017a.digitalWrite(rlyPins[3], rlyState[3] ? HIGH : LOW);
        updateLEDStatus(3);
        String stateMsg = "toggle:4:" + String(rlyState[3] ? 1 : 0);
        ws.textAll(stateMsg);
    }

    if (currentButtonStateIN5 && !lastButtonStateIN5) {
        // Toggle the relay state and send WebSocket message
        rlyState[4] = !rlyState[4];
        mcp23017a.digitalWrite(rlyPins[4], rlyState[4] ? HIGH : LOW);
        updateLEDStatus(4);
        String stateMsg = "toggle:5:" + String(rlyState[4] ? 1 : 0);
        ws.textAll(stateMsg);
    }

    if (currentButtonStateIN6 && !lastButtonStateIN6) {
        masterState = !masterState;
        mcp23017b.digitalWrite(LED8, masterState ? HIGH : LOW);
        // Send status message over serial
        char Message[10];
        snprintf(Message, sizeof(Message), "SET_SW3:%d", masterState ? 1 : 0);
        sendData(Message);
        // Send status over websocket
        _mar_status = (masterState == 1) ? "ON" : "OFF";  // Convert to text based on the value 1 or 0
        String mar = "{\"MASStatus\": \"" + _mar_status + "\"}";
        ws.textAll(mar);
    }

    if (currentButtonStateIN7 && !lastButtonStateIN7) {
        dnd_status = !dnd_status;
        mcp23017b.digitalWrite(LED2, dnd_status ? HIGH : LOW); 
        mcp23017b.digitalWrite(LED9, dnd_status ? HIGH : LOW);
        // Send status message over serial
        char Message[10];
        snprintf(Message, sizeof(Message), "SET_SW5:%d", dnd_status ? 1 : 0);
        sendData(Message);
        // Send status over websocket
        _dnd_status = (dnd_status == 1) ? "ON" : "OFF";
        String dnd = "{\"DNDStatus\": \"" + _dnd_status + "\"}";
        ws.textAll(dnd);
        }
    
    if (currentButtonStateIN8 && !lastButtonStateIN8) {
        rlyState[5] = !rlyState[5];
        mcp23017a.digitalWrite(rlyPins[5], rlyState[5] ? HIGH : LOW);
        mcp23017b.digitalWrite(LED10, rlyState[5] ? HIGH : LOW);
        updateLEDStatus(5); // To update on screen
        String stateMsg = "toggle:6:" + String(rlyState[5] ? 1 : 0);
        ws.textAll(stateMsg);
        }
    
    if (currentButtonStateIN9 && !lastButtonStateIN9) { // Detect button press
        balcony_status = !balcony_status; // Toggle the state
        rlyState[9] = !rlyState[9];
        mcp23017a.digitalWrite(rlyPins[9], rlyState[9] ? HIGH : LOW);
        // Control LEDs
        mcp23017b.digitalWrite(LED11, balcony_status ? HIGH : LOW);
        String stateMsg = "toggle:10:" + String(rlyState[9] ? 1 : 0);
        ws.textAll(stateMsg);
        }

    // Update the last button states
    lastButtonStateIN2 = currentButtonStateIN2;
    lastButtonStateIN3 = currentButtonStateIN3;
    lastButtonStateIN4 = currentButtonStateIN4;
    lastButtonStateIN5 = currentButtonStateIN5;

    lastButtonStateIN6 = currentButtonStateIN6;
    lastButtonStateIN7 = currentButtonStateIN7;
    lastButtonStateIN8 = currentButtonStateIN8;
    lastButtonStateIN9 = currentButtonStateIN9;
    
}

void updateLEDStatus(int relayState) {
    if (relayState >= 1 && relayState <= 4) {
        int ledPin = LED4 + (relayState - 1); // Calculate LED pin based on relayState
        mcp23017b.digitalWrite(ledPin, rlyState[relayState] ? HIGH : LOW);
    }
    else if(relayState >=5 && relayState <=6){
      // Create the status message

      int num = relayState - 4;
        char statusMessage[10];
        snprintf(statusMessage, sizeof(statusMessage), "SET_SW%d:%d", num, rlyState[relayState] ? 1 : 0);

        // Send the status message over serial
        sendData(statusMessage);
    }
    else if(relayState == 7){
      // Create the status message

      int num = relayState - 3;
        char statusMessage[10];
        snprintf(statusMessage, sizeof(statusMessage), "SET_SW%d:%d", num, rlyState[relayState] ? 1 : 0);

        // Send the status message over serial
        sendData(statusMessage);
    }
}

String wifi_signal(){
        Serial.print("RRSI: ");
        Serial.println(WiFi.RSSI());
        wifiSignal = WiFi.RSSI();
        return wifiSignal;
}

void readPresence(){
          // Read the state of the sensor (1 or 0)
            int pres_sensor_1 = digitalRead(pirPin);
            // int prValue2 = mcp23017b.digitalRead(IN9);

            // Interpret sensor value as ON or OFF
            if (pres_sensor_1 == HIGH || pres_sensor_2 == 1) {
              presenceStatus = "ON";
            } else {
              presenceStatus = "OFF";
            }

            // Print sensor status to serial monitor
            Serial.print("Sensor status: ");
            Serial.println(presenceStatus);
}

String readDHTTemperature() {
            // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
            // Read temperature as Celsius (the default)
            float t = dht.readTemperature();
            // Read temperature as Fahrenheit (isFahrenheit = true)
            //float t = dht.readTemperature(true);
            // Check if any reads failed and exit early (to try again).
            if (isnan(t)) {    
              Serial.println("Failed to read from DHT sensor!");
              return "--";
            }
            else {
              Serial.print("Temperature: ");
              Serial.println(t);
              current_temperature = (int)t;
              char tempReading[12];
                  snprintf(tempReading, sizeof(tempReading), "DIS_TEMP:%d", current_temperature);

                  // Send the status message over serial
                  sendData(tempReading);
              return String(t);
            }
}

String readDHTHumidity() {
            // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
            float h = dht.readHumidity();
            if (isnan(h)) {
              Serial.println("Failed to read from DHT sensor!");
              return "--";
            }
            else {
              Serial.print("Humidity: ");
              Serial.println(h);
              current_humidity = (int)h;
              char humidReading[12];
                  snprintf(humidReading, sizeof(humidReading), "DIS_HUMI:%d", current_humidity);
                  sendData(humidReading);
              return String(h);
            }
          }

void updateSensors(){
          if (ac_power == 1) {
              _ac_status = "ON";
          } else if (ac_power == 0) {
              _ac_status = "OFF";
          } else {
              _ac_status = "Status Error";
          }

          if (fan_mode == 1){
            _fan_mode = "LOW";
          }
          else if (fan_mode == 2){
            _fan_mode = "Med";
          } else if (fan_mode == 3){
            _fan_mode = "HIGH";
          } else {
            _fan_mode = "Status Error";
          }

          if (dnd_status == 0){
            _dnd_status = "OFF";
          }
          else if (dnd_status == 1){
            _dnd_status = "ON";
          }
          else {
            _dnd_status = "Status Error";
          }

          if (mur_status == 0){
            _mur_status = "OFF";
          } else if (mur_status == 1){
            _mur_status = "ON";
          }
          else {
            _mur_status = "Status Error";
          }

          if (masterState == 0){
            _mar_status = "OFF";
          } else if (masterState == 1){
            _mar_status = "ON";
          }
          else {
            _mar_status = "Status Error";
          }

          // Create JSON objects containing sensor data
          String tmp_reading = "{\"temperature\": \"" + readDHTTemperature() + "\"}";
          String hum_reading = "{\"humidity\": \"" + readDHTHumidity() + "\"}";
          String Presence_Status = "{\"presence\": \"" + presenceStatus + "\"}";
          String key_status = "{\"key\": \"" + keyStatus + "\"}";
          String KeyTag_Status = "{\"updatekey\": \"" + update_key + "\"}";
          String wifi_status = "{\"wifistatus\": \"" + wifiStatus + "\"}";
          String ip_Address = "{\"ipaddress\": \"" + ipAddress + "\"}";
          String wifi_signal = "{\"wifisignal\": \"" + wifiSignal + "\"}";
          String aircon_status = "{\"acstatus\": \"" + _ac_status + "\"}";
          String fan_speed = "{\"fanspeed\": \"" + _fan_mode + "\"}";
          String set_temp = "{\"setpoint\": \"" + String(set_point) + "\"}";
          String dnd = "{\"DNDStatus\": \"" + _dnd_status + "\"}";
          String mur = "{\"MURStatus\": \"" + _mur_status + "\"}";
          String mar = "{\"MASStatus\": \"" + _mar_status + "\"}";
          String mqtt_status = "{\"mqttstatus\": \"" + mqttstatus + "\"}";
          //Send presence status and range data to connected WebSocket clients
          ws.textAll(tmp_reading);
          ws.textAll(hum_reading);
          ws.textAll(Presence_Status);
          ws.textAll(key_status);
          ws.textAll(KeyTag_Status);
          ws.textAll(wifi_status);
          ws.textAll(ip_Address);
          ws.textAll(wifi_signal);
          ws.textAll(aircon_status);
          ws.textAll(fan_speed);
          ws.textAll(set_temp);
          ws.textAll(dnd);
          ws.textAll(mur);
          ws.textAll(mar);
          ws.textAll(mqtt_status);
}

void valve() {
    static bool lastState = rlyState[10]; // Track last state to prevent unnecessary updates
    bool stateChanged = false;

    // If AC power is OFF, turn off the valve relay
    if (ac_power == 0 && rlyState[10]) {
        rlyState[10] = false;
        stateChanged = true;
    }
    // Turn OFF Valve relay if temperature is too low
    else if (current_temperature <= (set_point - 1) && rlyState[10]) {
        rlyState[10] = false;
        stateChanged = true;
    }
    // Turn ON Valve relay if temperature is too high and AC is ON
    else if (current_temperature >= (set_point + 1) && ac_power == 1 && !rlyState[10]) {
        rlyState[10] = true;
        stateChanged = true;
    }

    // Update only if the state actually changed
    if (stateChanged && rlyState[10] != lastState) {
        lastState = rlyState[10];  // Save last state
        mcp23017a.digitalWrite(rlyPins[10], rlyState[10] ? HIGH : LOW);
        ws.textAll("toggle:11:" + String(rlyState[10] ? 1 : 0));
    }
}

void initFan(){
            ac_power = 1; // Set AC power to ON

            // send set_point data initially
            send_setpoint(default_set_point);
            
            // Read the current state of the KeyTag pin and save it to the variable
            noInterrupts(); // Disable interrupts
            keyTagState = digitalRead(keyTag);
            interrupts(); // Re-enable interrupts
              if (keyTagState == HIGH) {
                keyStatus = "IN";
                rlyState[11] = 1;
                rlyState[12] = 1;
                fan_mode = 2; // Medium
              } else {
                keyStatus = "OUT";
                rlyState[11] = 1;
                rlyState[12] = 0;
                fan_mode = 1; // Low
              }
              updateFanState();
              send_fanmode();
              send_acpower();
              send_keystatus();
            update_key = "Initial";
}

void send_keystatus(){
            char Message[20];
            snprintf(Message, sizeof(Message), "SET_KEY:%d", keyTagState);
            // Send the set_point over serial to display in the screen
            sendData(Message);
}

void send_setpoint(int value){
            set_point = value;
            char Message[20];
            snprintf(Message, sizeof(Message), "SET_POINT:%d", set_point);
            // Send the set_point over serial to display in the screen
            sendData(Message);
}

void send_fanmode(){
            char Message[20];
            snprintf(Message, sizeof(Message), "SET_FAN:%d", fan_mode);
            // Send the set_point over serial to display in the screen
            sendData(Message);
}

void send_acpower(){
            char Message[20];
            snprintf(Message, sizeof(Message), "SET_ACP:%d", ac_power);
            // Send the set_point over serial to display in the screen
            sendData(Message);
}

void blinkLED() {
  // Current time
  unsigned long currentLEDMillis = millis();

  // Check if it's time to blink the LED
  if (currentLEDMillis - previousLEDMillis >= LEDinterval) {
    // Save the last time you blinked the LED
    previousLEDMillis = currentLEDMillis;

    // Toggle the LED state
    mcp23017b.digitalWrite(LED1, !mcp23017b.digitalRead(LED1));
  }
}

void toggleRelay(int index, bool state) {
    rlyState[index] = state;
    mcp23017a.digitalWrite(rlyPins[index], state ? HIGH : LOW);
    String stateMsg = "toggle:" + String(index + 1) + ":" + String(state ? 1 : 0);
    ws.textAll(stateMsg);
}

void call_start() {
    blinkLED();
    
    if (!key_send){
      send_keystatus();
      key_send = true;
    }

     // Change Fan Mode to Med
    if (!fanModeOverride) {
        rlyState[11] = 1;
        rlyState[12] = 1;
        updateFanState();
        fan_mode = 2;  // Medium mode
        send_fanmode();
        fanModeOverride = true;
    }

    if (!setPointInitialized) {  // Run only once
        set_point = init_set_point; // Initial set point when card inserted
        send_setpoint(set_point); // To update in the display screen
        setPointInitialized = true; // Prevent further execution
    }

    if (!rlyState[13]) {
        toggleRelay(13, true);
    }

    if (!rlyState[8] && masterState == 0) {
        toggleRelay(8, true);
    } else if (rlyState[8] && masterState == 1) {
        for (int i = 1; i <= 9; i++) {
            if (rlyState[i]) {
                toggleRelay(i, false);
                updateLEDStatus(i);
                delay(5);
            }
        }
        mcp23017b.digitalWrite(LED8, LOW); // living room panel - master indicator
        mcp23017b.digitalWrite(LED10, LOW); // living room panel - pendant indicator
        mcp23017b.digitalWrite(LED11, LOW); // writing table panel - balcony indicator
    }
}

void call_stop() {

    if (callStopExecuted) return; // Exit if already executed

    key_send = false;
    send_keystatus();
    
    masterState = 0;
    set_point = default_set_point;
    send_setpoint(set_point); // to update in the display screen
    setPointInitialized = false; // linked in call_start function

    if (rlyState[13]) {
        toggleRelay(13, false);
    }

    // Change Fan Mode to Low
    rlyState[11] = 1;
    rlyState[12] = 0;
    updateFanState();
    fan_mode = 1;
    send_fanmode();
    ac_power = 1;
    send_acpower();
    fanModeOverride = false;

    for (int i = 1; i <= 9; i++) {
            if (rlyState[i]) {
                toggleRelay(i, false);
                updateLEDStatus(i);
            }
        }
    mcp23017b.digitalWrite(LED1, LOW); // Door Panel LED should be OFF
    mcp23017b.digitalWrite(LED8, LOW); // Living room panel master led should be off
    mcp23017b.digitalWrite(LED10, LOW); // Living room panel Pendant led should be off
    mcp23017b.digitalWrite(LED11, LOW); // Writing table panel Balcony led should be off

    int j = 3;
      char statusMessage[10];
        snprintf(statusMessage, sizeof(statusMessage), "SET_SW%d:%d", j, 0);

        // Send the status message over serial to update in the display screen
        sendData(statusMessage);   // to update the master button in the display screen

        callStopExecuted = true; // Mark as executed
}

void mqttPublish(){
  
    // Get ESP32 MAC address
    String macAddress = WiFi.macAddress();

    // Get ESP32 IP address
    String ip_address = WiFi.localIP().toString();

    // Get Wi-Fi signal strength
    int32_t rssi = WiFi.RSSI();

    String _temp = readDHTTemperature();
    String _hum = readDHTHumidity();

    // Create a message payload
    String payload = "MAC: " + macAddress + ", IP: " + ip_address + ", Signal: " + String(rssi) + ", AC: " + _ac_status + ", FAN: " + _fan_mode + ", DND: " + _dnd_status + ", MUR: " + _mur_status + ", MASTER: " + _mar_status + ", Presence: " + presenceStatus + ", Key: " + keyStatus + ", Temperature: " + _temp + ", Humidity: " + _hum;

    mqttClient.publish(publishTopic, payload.c_str());

    Serial.println("Message published: " + payload);
}

void _checkInterrupt(){
if (keyTagState == HIGH){
  call_start();
  callStopExecuted = false; // Mark as executed
}
else if (keyTagState == LOW){
  call_stop();
}
}

/* ################ Setup ################ */

void setup(){
  
  Serial.begin(115200);
  SerialPort.begin(115200, SERIAL_8N1, SERIAL_RX_PIN, SERIAL_TX_PIN); // For thermostat communication
  
  pinMode(WiFiRST_BUTTON, INPUT_PULLUP); // Use internal pull-up resistor

  pinMode(keyTag, INPUT);
  attachInterrupt(digitalPinToInterrupt(keyTag), handleInterrupt, CHANGE); // Interrupt initialization

  pinMode(pirPin, INPUT_PULLDOWN);

  dht.begin(); // initialize dht sensor

  Wire.begin();     // initialize I2C serial bus
  mcp23017a.init();  // initialize MCP23017a IC
  mcp23017b.init();  // initialize MCP23017b IC


  // Configure MCP23017 I/O pins
  configureMCPaPinMode();  // familiar pinMode() style
  configureMCPbPinMode();  // familiar pinMode() style
  // Reset MCP23017 ports
  mcp23017a.writeRegister(MCP23017Register::GPIO_A, 0x00);
  mcp23017a.writeRegister(MCP23017Register::GPIO_B, 0x00);

  mcp23017b.writeRegister(MCP23017Register::GPIO_A, 0x00);
  mcp23017b.writeRegister(MCP23017Register::GPIO_B, 0x00);

  // Initialize LittleFS
  if(!LittleFS.begin()){
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
  }

    File root = LittleFS.open("/");
    File file = root.openNextFile();
  
  while(file){
    Serial.print("File: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }

  // Load set_point from the file
    default_set_point = readSetPointFromFile(setPointFilePath);
    Serial.print("Loaded default set point: ");
    Serial.println(default_set_point);

  // wifi config
    loadWiFiSettings();

  // Check if wifireset button is pressed on boot for 5 seconds.
    checkWiFireset();

  // Websocket config  
    ws.onEvent(eventHandler);
    server.addHandler(&ws);

  // Define routes
        server.on("/settings/set_point.txt", HTTP_GET, [](AsyncWebServerRequest *request){
              request->send(LittleFS, "/settings/set_point.txt", "text/txt");
    });

        server.on("/images/ban.png", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(LittleFS, "/images/ban.png", "image/png");
    });

        server.on("/images/broom.png", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(LittleFS, "/images/broom.png", "image/png");
    });

        server.on("/images/key.png", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(LittleFS, "/images/key.png", "image/png");
    });

        server.on("/images/snowflake.png", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(LittleFS, "/images/snowflake.png", "image/png");
    });

        server.on("/images/thermometer-half.png", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(LittleFS, "/images/thermometer-half.png", "image/png");
    });

        server.on("/images/wifi.png", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(LittleFS, "/images/wifi.png", "image/png");
    });

        server.on("/images/user.png", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(LittleFS, "/images/user.png", "image/png");
    });

        server.on("/script/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send(LittleFS, "/script/script.js", "application/javascript");
    });
    
        server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/style.css", "text/css");
    });

        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

        // Scan wifi using default ip and url while on AP Mode http://192.168.4.1/scan 
        // IP 192.168.4.1 is hardcoded in the ESP-IDF for AP Mode
        server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
           isScanning = true;
        int n = WiFi.scanComplete();  // Use scanComplete() to check results
        String networks = "[";

        if (n > 0) {  // Only process if networks are found
            for (int i = 0; i < n; ++i) {
                networks += "\"" + WiFi.SSID(i) + "\",";
            }
            networks.remove(networks.length() - 1);  // Remove last comma
        }

        networks += "]";  // Close JSON array
        request->send(200, "application/json", networks);
        
        // Start a new async scan for next request
        WiFi.scanNetworks(true, false);
    });

        // Route for saving the wifi settings
        server.on("/save", HTTP_POST, saveNewWiFiSettings); 

    // Starts webserver
    server.begin();

    
    if (WiFi.status() == WL_CONNECTED){
        wifiStatus = "Connected";
       // Load MQTT settings from NVS
        MQTTConfig mqttConfig = loadMQTTSettings();

        // Connect to MQTT broker if settings exist
        if (mqttConfig.isValid) {
            connectMQTT(mqttConfig);
        } else {
            Serial.println("⚠️ MQTT settings not found! Please configure via WebSocket.");
        }
        
        // Initialize and get the time
        connectTimeServer();
        timeSync(); 
    } 
    
    startTime = millis();   // Used in AC Switch ON function called after a powercycle. 
}

/* ################ Loop ################ */

void loop() {
      // Continuously call serialEvent to check for new data
      serialEvent();

      // Continuously check if interrupt flag is updated
      _checkInterrupt();

      // Continuously check switch panels events
      switchPanel();
      
      // Check WiFi connection periodically
      checkWiFiStatus();

      // Get the current milliseconds
      unsigned long currentMillis = millis();

      // Initialize Fan Mode after rebooting.
      if (!fanInitilized){
        unsigned long elapsedTime = currentMillis - startTime;
        if (ac_power == 0 && elapsedTime >= startDelay) {
            initFan();
            fanInitilized = true;
          }
        }

      // Check frequent status of wifi signal, sensors and valve relay
      if (currentMillis - previousMillis >= interval) {
      // Update the previous timestamp
      previousMillis = currentMillis;
      // Process data
      wifi_signal();
      readPresence();
      updateSensors();
      valve();
      }

      // Check if WiFi is connected
      if (WiFi.status() == WL_CONNECTED){
          mqtt_cb();
          // Ensure time stays synced     
          timeSync();
          sendTimeToDisplay(); // Send time to display every minute
      }
      
      // To clear websocket idle clients
      if (millis() - lastCleanupTime >= cleanupInterval) {
          ws.cleanupClients();
          lastCleanupTime = millis();
          }
      // Send a ping to all clients every 10 seconds to keep the connection alive
      static unsigned long lastPingTime = 0;
      if (millis() - lastPingTime >= pingInterval) {
          Serial.println("Sending ping to clients...");
          ws.pingAll(); // Send a "ping" frame to all connected WebSocket clients
          lastPingTime = millis(); // Reset ping timer
      }
}