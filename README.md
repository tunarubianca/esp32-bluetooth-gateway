# esp32-bluetooth-gateway
Bluetooth and Wi-Fi gateway program for ESP32 with external REST API integration

# Features
Processes real-time serial commands (`GetNetworks`, `Connect <ssid> <pass>`, `GetData`, `GetDetails <id>`).
Scans 2.4 GHz wireless networks and serializes dynamic connection status flags.
Connects to a REST API via WiFi to download character data in JSON format using the HTTPClient and ArduinoJson libraries.
Uses a custom C++ helper function to clean up and format the character details (Movies, TV Shows, Allies, Enemies) into simple text before sending it to the mobile app via Bluetooth.
