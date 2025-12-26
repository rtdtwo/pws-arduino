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


// --------------------------------------------------------------------------------
// SENSOR SETUP
// --------------------------------------------------------------------------------

// DPS310 Sensor - Air Pressure & Temperature
Adafruit_DPS310 dpsSensor;

// DHT22 Sensor - Temperature & Humidity
DHT dht(2, DHT22);


// --------------------------------------------------------------------------------
// DATA PERSISTENCE
// --------------------------------------------------------------------------------

class Measurement {
  public:
    float value = -100.0f;
    int index = -1;
    String dataType = "";
    bool isPopulated = false;

    Measurement() = default;

    Measurement(float v, int i, String t)  {
      value = v;
      index = i;
      dataType = t;
      isPopulated = true;
      Serial.println("PERSIST: " + String(v) + " // " + String(i) + " // " + t);
    }
};

// Calculate how many data points to persist based on measurement wait time.
// We want to measure every <<MEASUREMENT_WAIT_TIME>> minutes, divide by <<MEASUREMENT_WAIT_TIME>>
// and then, since we measure temperature, humidity, and pressure, multiply by 3
int persistenceCapacity = (<<PUBLISH_WAIT_TIME>> / <<MEASUREMENT_WAIT_TIME>>) * 3;
Measurement measurements[persistenceCapacity];
int measurementIndex = 0;

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
  int maxLoops = persistenceCapacity / 3;
  int waitTime = <<MEASUREMENT_WAIT_TIME>> * 60 * 1000; // <<MEASUREMENT_WAIT_TIME>> minutes
  delay(waitTime);

  // Read from DHT22 sensor
  float dhtHumidityReading = dht.readHumidity();
  float dhtTemperatureReading = dht.readTemperature();

  // Read from DPS310 sensor
  float dpsPressureReading = NAN;
  float dpsTemperatureReading = NAN;
  sensors_event_t temp_event, pressure_event;
  if (dpsSensor.temperatureAvailable() && dpsSensor.pressureAvailable()) {
    dpsSensor.getEvents(&temp_event, &pressure_event);
    dpsPressureReading = pressure_event.pressure;
    dpsTemperatureReading = temp_event.temperature;
  }

  int basePersistenceArrayIndex = measurementIndex * 3;

  if (!isnan(dhtHumidityReading)) {
    measurements[basePersistenceArrayIndex] = Measurement(dhtHumidityReading, measurementIndex, "HUMIDITY");
  }

  if (!isnan(dhtTemperatureReading) && !isnan(dpsTemperatureReading)) {
    float tempToReport = ((dpsTemperatureReading + dhtTemperatureReading) / 2.0f);
    measurements[basePersistenceArrayIndex + 1] = Measurement(tempToReport, measurementIndex, "TEMPERATURE");
  }

  if (!isnan(dpsPressureReading)) {
    measurements[basePersistenceArrayIndex + 2] = Measurement(dpsPressureReading, measurementIndex, "PRESSURE");
  }

  if (measurementIndex == (maxLoops - 1)) {
    checkWifiConnectionAndReconnectIfNeeded();
    bool shouldSendData = false;

    DynamicJsonDocument requestDoc(2048);
    requestDoc["source_id"] = sourceId;
    JsonArray dataArray = requestDoc.createNestedArray("data");

    for (int i = 0; i < persistenceCapacity; i++) {
      Measurement m = measurements[i];
      if (m.isPopulated) {
        shouldSendData = true;
        buildSensorDataJson(dataArray.add<JsonObject>(), m.index, m.value, m.dataType);
      }
    }

    // Don't send a POST request if there's no data
    if (shouldSendData) {
      sendPostRequest(requestDoc);
    } else {
      Serial.println("ERROR: No sensor returned any reportable value!");
    }

    resetMeasurementArray();
    measurementIndex = 0;
  } else {
    measurementIndex = measurementIndex + 1;
  }
}

void resetMeasurementArray() {
  for (int i = 0; i < persistenceCapacity; i++) {
    measurements[i] = Measurement();
  }
}

void buildSensorDataJson(JsonObject obj, int index, float value, String dataType) {
  obj["index"] = index;
  obj["value"] = value;
  obj["type"] = dataType;
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
  // Dummy read to clear the sensor's internal register
  dht.readHumidity();
  dht.readTemperature();
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
