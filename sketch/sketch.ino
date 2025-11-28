#include <WiFiS3.h>
#include <DHT.h>
#include <ArduinoJson.h>


// --------------------------------------------------------------------------------
// CUSTOMIZABLE CONFIGURATION
// --------------------------------------------------------------------------------

// WiFi Credentials
char ssid[] = "<<WIFI_SSID>>";        // your network SSID (name)
char pass[] = "<<WIFI_PASSWORD>>";    // your network password (use for WPA, or use as key for WEP)

// Server Endpoint Configuration
const char* serverAddress = "<<SERVER_ADDRESS>>";  // server address (no protocol)
const int serverPort = 443;                        // HTTPS port
const char* endpoint = "<<SERVER_WRITE_ENDPOINT>>";         // API endpoint path

// API Authentication
const char* apiKey = "<<SERVER_WRITE_API_KEY>>";

// Source ID
const String sourceId = "<<SOURCE_ID>>";

// DHT Sensor Configuration
#define DHTPIN 2     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// --------------------------------------------------------------------------------

DHT dht(DHTPIN, DHTTYPE);
WiFiSSLClient client; // Using WiFiSSLClient for HTTPS
int status = WL_IDLE_STATUS;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect
  }
  Serial.println("DHT22 Reader Starting...");
  
  // Initialize DHT sensor
  dht.begin();
  Serial.println("DHT22 sensor initialized");

  // Check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("ERROR: WiFi module not found!");
    // Communication with WiFi module failed
    while (true);
  }

  // Attempt to connect to WiFi network:
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  while (status != WL_CONNECTED) {
    // Connect to WPA/WPA2 network.
    status = WiFi.begin(ssid, pass);
    Serial.print(".");
    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println();
  Serial.println("WiFi connected successfully!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Wait some time between measurements.
  int waitTime = 2 * 60 * 1000; //  2 minutes
  delay(waitTime);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if any reads failed - if so, skip sending data
  if (isnan(h) || isnan(t)) {
    Serial.println("ERROR: Failed to read from DHT sensor!");
    return; // Don't send POST request on error
  }

  Serial.println("--- Sensor Reading ---");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println(" %");
  Serial.println("---------------------");

  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
      Serial.println("ERROR: WiFi disconnected! Attempting to reconnect...");
      // Try to reconnect
      status = WiFi.begin(ssid, pass);
      if (status != WL_CONNECTED) {
          Serial.println("ERROR: WiFi reconnection failed!");
          // If still not connected, we can't send data.
          return; 
      }
      Serial.println("WiFi reconnected successfully");
  }

  DynamicJsonDocument requestDoc(256);
  // Put source ID
  requestDoc["source_id"] = sourceId;

  JsonArray dataArray = requestDoc.createNestedArray("data");
  buildSensorDataJson(dataArray.add<JsonObject>(), t, "TEMPERATURE");
  buildSensorDataJson(dataArray.add<JsonObject>(), h, "HUMIDITY");

  // Send the post request
  sendPostRequest(requestDoc);
}

void buildSensorDataJson(JsonObject obj, float value, String type) {
  obj["value"] = value;
  obj["type"] = type;
}

void sendPostRequest(JsonDocument data) {
  Serial.println();
  Serial.print("Sending POST request");
  Serial.println("...");
  
  if (client.connect(serverAddress, serverPort)) {
    Serial.println("Connected to server");
    
    // Construct JSON payload
    String postData;
    serializeJson(data, postData);

    Serial.print("Request payload: ");
    Serial.println(postData);

    // Make a HTTP request:
    client.print("POST ");
    client.print(endpoint);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(serverAddress);
    client.println("Content-Type: application/json");
    client.print("X-API-Key: ");
    client.println(apiKey);
    client.print("Content-Length: ");
    client.println(postData.length());
    client.println("Connection: close");
    client.println();
    client.println(postData);
    
    Serial.println("Request sent, waiting for response...");
  } else {
    Serial.println("ERROR: Connection to server failed!");
    return;
  }
  
  // Read response status line
  Serial.println("--- HTTP Response ---");
  bool headersComplete = false;
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      headersComplete = true;
      break;
    }
    Serial.println(line); // Print response headers
  }
  
  // Read the response body
  if (headersComplete) {
    Serial.println("Response body:");
    String responseBody = "";
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
      responseBody += c;
    }
    if (responseBody.length() == 0) {
      Serial.println("(empty)");
    }
  }
  Serial.println("\n--------------------");
  
  client.stop();
  Serial.println("Connection closed");
}
