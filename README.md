# 🌤️ Rishabh's PWS - Arduino
This repository contains the Arduino source code for my DIY personal weather station project. It is divided into two main components: a Python-based utility suite for configuration and logging, and an Arduino sketch for sensor data collection and transmission.

## Repository Structure

* **/python**: Contains scripts for local data logging and environment-based sketch configuration.
* **/arduino**: Contains the C++ code (sketch) for the Arduino microcontroller.

## Python Utilities
The Python component manages the interaction between your computer and the Arduino, as well as preparing the code for deployment.

### Scripts
* **`read_logs.py`**: Connects to the Arduino via serial to read and save incoming sensor logs to a local file.
* **`apply_env_to_sketch.py`**: A pre-processing script. It reads the Arduino sketch template, replaces environment variable placeholders (e.g., API keys, WiFi credentials) with values from a `.env` file, and generates a ready-to-upload `.ino` file.

### Environment Configuration
Create a `.env` file in the `/python` directory. Include the following variables:
```env
# WiFi credentials
WIFI_SSID=your_wifi_ssid
WIFI_PASSWORD=your_wifi_password

# API server details
SERVER_ADDRESS=yourApiServer.com
SERVER_WRITE_ENDPOINT=/the/post/request/endpoint
SERVER_WRITE_API_KEY=an-api-key-assigned-to-your-sensor

# Sensor details
SOURCE_ID=a-uuid-assigned-to-your-sensor

# How long to wait between each measurements (in minutes)
MEASUREMENT_WAIT_TIME=5

# How long to wait between each POST request (in minutes)
PUBLISH_WAIT_TIME=15
```

## Arduino Sketch

The Arduino sketch handles the hardware logic. It reads data from connected sensors and publishes that data to a database using a REST API. The sketch requires a WiFi enabled base board, like the Arduino UNO R4 Wifi.

### Features
* **Sensor Integration**: Reads temperature, humidity, and atmospheric pressure.
* **Data Transmission**: Sends a JSON payload via an HTTP POST request to a configurable endpoint.
* **Serial Feedback**: Outputs real-time status and errors for the Python logger.

### Data Workflow

1. **Arduino** reads sensors every `MEASUREMENT_WAIT_TIME` minutes and persists the data.
2. After every `PUBLISH_WAIT_TIME` minutes, **Arduino** creates a JSON payload and sends **POST** request to the configured REST API.
   
### Setup
1. **Dependencies**: Ensure you have the **Arduino IDE** installed.
3. **Library Installation**: You will need to install the following libraries via the Arduino Library Manager:
* `DHT sensor library`
* `Adafruit DPS310`
* `ArduinoJson`

3. **Compile and Upload**:
* Run the Python `apply_env_to_sketch.py` first to generate a processed `.ino` file.
* Open the generated file in the Arduino IDE.
* Select your board and port, then click **Upload**.
