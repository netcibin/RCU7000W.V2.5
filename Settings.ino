
Preferences prefwifi;

const char *defaultAPSSID = "RCU7000W";
const char *defaultAPPass = "1122334455";

String ssid = "";
String password = "";

bool wifistamode = false;

bool wifiConnecting = false;
unsigned long wifiAttemptStartTime = 0;

// Load Wi-Fi credentials from NVS
void loadWiFiSettings() {
        prefwifi.begin("wifi", true); // Read-only mode
        ssid = prefwifi.getString("ssid", "");
        password = prefwifi.getString("password", "");
        prefwifi.end();

        if (ssid == "" || password == "") {
            setupAPMode();
        } else {
            setupStationMode();
        }
}

// Initialize Access Point Mode
void setupAPMode() {
        Serial.println("Starting in AP mode.");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(defaultAPSSID, defaultAPPass);
        Serial.println("AP Mode Initialized");
}

void setupStationMode(){
        Serial.println("Connecting...");
        WiFi.mode(WIFI_STA);  // Set to Station Mode
        WiFi.begin(ssid.c_str(), password.c_str());
        wifistamode = true;
}

void clearWiFiSettings() {
        prefwifi.begin("wifi", false); // Open in write mode
        prefwifi.clear();  // Erase all saved Wi-Fi settings
        prefwifi.end();
        Serial.println("Wi-Fi settings cleared!");
}

// Save Wi-Fi credentials to NVS
void saveWiFiSettings(String newSSID, String newPassword) {
        prefwifi.begin("wifi", false); // Write mode
        prefwifi.putString("ssid", newSSID);
        prefwifi.putString("password", newPassword);
        prefwifi.end();
        Serial.println("Wi-Fi settings saved!");
}

// update wifi from Web API
void saveNewWiFiSettings(AsyncWebServerRequest *request) {
    if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
        ssid = request->getParam("ssid", true)->value();
        password = request->getParam("password", true)->value();
        
        saveWiFiSettings(ssid, password);
        request->send(200, "text/plain", "Saved. Restarting...");
        delay(2000);
        ESP.restart();
    } else {
        request->send(400, "text/plain", "Invalid Request");
    }
}

void connectToWiFi() {
    if (!wifiConnecting) {
        WiFi.disconnect();
        setupStationMode();
        wifiAttemptStartTime = millis();
        wifiConnecting = true;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected.");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        wifiStatus = "Connected";
        ipAddress = WiFi.localIP().toString();
        
        MDNS.end();
        if (MDNS.begin("rcu7000w")) {
            Serial.println("mDNS responder started");
        } else {
            Serial.println("Error setting up MDNS responder!");
        }

        wifiConnecting = false;
        connectTimeServer();
    }
    else if (millis() - wifiAttemptStartTime >= 10000) {
        Serial.println("\nFailed to connect to WiFi.");
        wifiStatus = "Disconnected";
        wifiConnecting = false;
    }
}

void checkWiFiStatus() {
    if (WiFi.getMode() == WIFI_AP || isScanning) {
        return; // Skip WiFi checking if in AP mode
    }

    if (millis() - previouswifiMillis >= checkwifiInterval) {
        previouswifiMillis = millis();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] Disconnected! Reconnecting...");
            if (!isScanning){  // Double-check before reconnecting
              connectToWiFi();
            }
            
        }
    }
/*
    // Check if reset button is pressed
    if (digitalRead(WiFiRST_BUTTON) == LOW) {
        Serial.println("Reset button pressed! Switching to AP mode...");
        clearWiFiSettings();
        setupAPMode();
    }
    */
    checkWiFireset();
}

void checkWiFireset() { 
    static unsigned long pressStartTime = 0;
    static bool resetTriggered = false;
    static bool buttonWasPressed = false;

    if (digitalRead(WiFiRST_BUTTON) == LOW) { 
        if (!buttonWasPressed) {  
            pressStartTime = millis();  // Start timing only once when first pressed
            buttonWasPressed = true;
        }

        if (millis() - pressStartTime >= WiFiRST_HOLD_TIME && !resetTriggered) {
            Serial.println("WiFi reset button held for 5 seconds! Switching to AP mode...");
            clearWiFiSettings();
            setupAPMode();
            resetTriggered = true;  // Prevent repeated triggers
        }
    } else {
        buttonWasPressed = false;  // Reset when button is released
        resetTriggered = false;
    }
}
