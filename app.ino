#include <WiFi.h>
#include <BluetoothSerial.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

BluetoothSerial BT;

String teamId = "38";  
String incoming = "";
bool cmdReady = false;

HTTPClient http;

StaticJsonDocument<256> jsonOut;

void handleGetNetworks();
void handleConnect(const char* ssid, const char* password);
void handleGetData();
void handleGetDetails(const String& id);

void setup() {
  Serial.begin(115200);
  BT.begin("ESP32");
  Serial.println("Bluetooth pornit");
  BT.println("Comenzi acceptate: GetNetworks, Connect <ssid> <password>, GetData, GetDetails <id>");
}

void loop() {
  while (BT.available()) {
    char c = BT.read();
    if (c == '\n') {
      cmdReady = true;
    } else if (c != '\r') {
      incoming += c;
    }
  }

  if (cmdReady) {
    incoming.trim();

    if (incoming == "GetNetworks") {
      handleGetNetworks();
    } 
    else if (incoming.startsWith("Connect")) {
      int space1 = incoming.indexOf(' ');
      int space2 = incoming.indexOf(' ', space1 + 1);
      if (space2 != -1) {
        String ssid = incoming.substring(space1 + 1, space2);
        String password = incoming.substring(space2 + 1);
        handleConnect(ssid.c_str(), password.c_str());
      } else {
        BT.println("{\"error\":\"Comanda CONNECT trebuie să conțină SSID și parolă.\"}");
      }
    } 
    else if (incoming == "GetData") {
      handleGetData();
    } 
    else if (incoming.startsWith("GetDetails")) {
     String id = incoming.substring(strlen("GetDetails") + 1); 
      handleGetDetails(id);
    } 
    else if (incoming == "GetTeamID") {
      jsonOut.clear();
      jsonOut["teamId"] = teamId;
      serializeJson(jsonOut, BT);
      BT.println();
    } 
    else {
      BT.println("{\"error\":\"Comandă necunoscută\"}");
    }

    incoming = "";
    cmdReady = false;
  }
}

void handleGetNetworks() {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    jsonOut.clear();
    jsonOut["ssid"] = WiFi.SSID(i);
    jsonOut["strength"] = WiFi.RSSI(i);
    jsonOut["encryption"] = WiFi.encryptionType(i);
    jsonOut["teamId"] = teamId;
    serializeJson(jsonOut, BT);
    BT.println();
  }
}

void handleConnect(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  Serial.print("Conectare la ");
  Serial.println(ssid);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    BT.print(".");
  }

  jsonOut.clear();
  jsonOut["ssid"] = ssid;
  jsonOut["connected"] = (WiFi.status() == WL_CONNECTED);
  serializeJson(jsonOut, BT);
  BT.println();

  Serial.println("Trimis JSON:");
  serializeJson(jsonOut, Serial);
  Serial.println();

  delay(100);
  BT.println("gata");
  Serial.println("gata");
}

void handleGetData() {
  if (WiFi.status() != WL_CONNECTED) {
    BT.println("{\"error\":\"ESP32 nu este conectat la WiFi\"}");
    return;
  }

  String apiUrl = "http://proiectia.bogdanflorea.ro/api/disney-characters/characters";
  http.begin(apiUrl);
  http.setTimeout(8000);
  int httpCode = http.GET();

  if (httpCode == 200) {
    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, http.getString());

    if (err) {
      BT.print("{\"error\":\"Deserializare eșuată: ");
      BT.print(err.c_str());
      BT.println("\"}");
      return;
    }

    for (JsonObject rec : doc.as<JsonArray>()) {
      jsonOut.clear();
      jsonOut["id"] = rec["_id"];
      jsonOut["name"] = rec["name"];
      jsonOut["image"] = rec["imageUrl"];

      String formatted;
      serializeJsonPretty(jsonOut, formatted);
      BT.println(formatted);
    }
  } else {
    jsonOut.clear();
    jsonOut["error"] = "HTTP " + String(httpCode);
    String formatted;
    serializeJsonPretty(jsonOut, formatted);
    BT.println(formatted);
  }

  http.end();
}

void handleGetDetails(const String& id) {
  if (WiFi.status() != WL_CONNECTED) {
    BT.println("{\"error\":\"ESP32 nu este conectat la WiFi\"}");
    return;
  }

  String url = "http://proiectia.bogdanflorea.ro/api/disney-characters/character?_id=" + id;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, http.getString());

    if (err) {
      BT.print("{\"error\":\"Deserializare eșuată: ");
      BT.print(err.c_str());
      BT.println("\"}");
      return;
    }

    jsonOut.clear();
    jsonOut["id"] = doc["_id"] | id;
    jsonOut["name"] = doc["name"] | "Necunoscut";
    jsonOut["image"] = doc["imageUrl"] | "";

    String formatted;
    serializeJsonPretty(jsonOut, formatted);
    BT.println(formatted);

    String desc = "";

    auto appendList = [&](const char* label, JsonArray arr) {
      if (!arr.isNull() && arr.size() > 0) {
        desc += "• " + String(label) + ": ";
        for (JsonVariant v : arr) {
          desc += v.as<String>() + ", ";
        }
        desc.remove(desc.length() - 2);
        desc += "\n";
      }
    };

    appendList("Filme", doc["films"]);
    appendList("Scurtmetraje", doc["shortFilms"]);
    appendList("Seriale TV", doc["tvShows"]);
    appendList("Jocuri Video", doc["videoGames"]);
    appendList("Apariții în Parcuri", doc["parkAttractions"]);
    appendList("Apropiați", doc["allies"]);
    appendList("Dușmani", doc["enemies"]);

    if (desc == "") {
      desc = "Nu există informații suplimentare.";
    }

    BT.println(desc);
  }

  http.end();
}
