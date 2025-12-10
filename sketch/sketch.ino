#include <WiFiS3.h>
#include <DHT.h>
#include <Adafruit_DPS310.h>
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

// DPS310 Sensor - Air Pressure & Temperature
Adafruit_DPS310 dpsSensor;

// DHT22 Sensor - Temperature & Humidity
DHT dht(2, DHT22);

// --------------------------------------------------------------------------------

WiFiSSLClient client; // Using WiFiSSLClient for HTTPS
int status = WL_IDLE_STATUS;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect
  }
  
  // Initialize DHT sensor
  setupDHTSensor();

  // Initialize DPS310 sensor
  setupDPSSensor();

  // Initialize WiFi
  setupWifi();
}

void loop() {
  // Wait some time between measurements.
  int waitTime = 3 * 60 * 1000; //  3 minutes
  delay(waitTime);

  // Read from DHT22 sensor
  float dhtHumidityReading = dht.readHumidity();
  float dhtTemperatureReading = dht.readTemperature();

  // Read from DPS310 sensor
  float dpsPressureReading;
  float dpsTemperatureReading;
  sensors_event_t temp_event, pressure_event;
  if (dpsSensor.temperatureAvailable() && dpsSensor.pressureAvailable()) {
    dpsSensor.getEvents(&temp_event, &pressure_event);
    dpsPressureReading = pressure_event.pressure;
    dpsTemperatureReading = temp_event.temperature;
  }

  // Don't send a POST request if there's no data
  if (isnan(dhtHumidityReading) && isnan(dpsTemperatureReading) && isnan(dpsPressureReading)) {
    Serial.println("ERROR: No sensor returned any reportable value!");
    return;
  }

  Serial.println("--- Sensor Readings ---");
  Serial.print("DHT22 Temperature: ");
  Serial.print(dhtTemperatureReading);
  Serial.println("°C");
  Serial.print("DHT22 Humidity: ");
  Serial.print(dhtHumidityReading);
  Serial.println("%");
  Serial.print("DPS310 Pressure: ");
  Serial.print(dpsPressureReading);
  Serial.println(" mb");
  Serial.print("DPS310 Temperature: ");
  Serial.print(dpsTemperatureReading);
  Serial.println("°C");
  Serial.println("---------------------");

  checkWifiConnectionAndReconnectIfNeeded();

  DynamicJsonDocument requestDoc(256);
  // Put source ID
  requestDoc["source_id"] = sourceId;

  JsonArray dataArray = requestDoc.createNestedArray("data");

  // Include Temperature only if it is not null
  if (!isnan(dpsTemperatureReading)) {
    buildSensorDataJson(dataArray.add<JsonObject>(), dpsTemperatureReading, "TEMPERATURE");
  }

  // Include Temperature only if it is not null
  if (!isnan(dhtHumidityReading)) {
    buildSensorDataJson(dataArray.add<JsonObject>(), dhtHumidityReading, "HUMIDITY");
  }

  // Include pressure only if it is not null or zero
  if (!isnan(dpsPressureReading)) {
    buildSensorDataJson(dataArray.add<JsonObject>(), dpsPressureReading, "PRESSURE");
  }

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


void setupWifi() {
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
    // wait 10 seconds for connection
    delay(10000);
  }
  Serial.println();
  Serial.println("WiFi connected successfully!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void setupDHTSensor() {
  dht.begin();
  Serial.println("DHT22 sensor initialized");
}


void setupDPSSensor() {
  if (!dpsSensor.begin_I2C()) {
    Serial.println("Failed to initialize DPS310 sensor");
  }
  Serial.println("DPS310 sensor initialized");
  delay(1000);
}

void checkWifiConnectionAndReconnectIfNeeded() {
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
}
