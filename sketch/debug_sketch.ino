#include <DHT.h>
#include <Adafruit_DPS310.h>

// DPS310 Sensor - Air Pressure & Temperature
Adafruit_DPS310 dpsSensor;

// DHT22 Sensor - Temperature & Humidity
DHT dht(2, DHT22);


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
}


void loop() {
  // Wait 5 seconds for each measurement
  delay(5000);
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