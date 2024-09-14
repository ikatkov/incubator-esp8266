#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include "build_version.h"

/* WIFI SSID & Password + InfluxDB configs */
#include "credentials.h"
// raw data copied into a string on each build
#include "index_html.h"

#define TZ_INFO "UTC-8"
// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient influxDBClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
// Declare Data point
Point measurementPoint("incubator-esp8266");

#define BUZZER_PIN D0
#define DS1820_PIN D1
#define HEATER_PIN D2
#define TEMP_CHECK_INTERVAL 1000
#define WIFI_RETRY_INTERVAL 60000
#define EEPROM_ADDRESS 0
#define MAX_BATCH_SIZE 60
#define WRITE_BUFFER_SIZE 120
#define WRITE_PRECISION WritePrecision::S

OneWire oneWire(DS1820_PIN);
DallasTemperature sensors(&oneWire);

u_int64_t lastTempReadingMs;
u_int64_t lastWifiRetryMs;
float temp = 0;
u8_t setTemp = 0;
String state = "Idle";

ESP8266WebServer server(80);
JsonDocument responseJSON;
JsonDocument versionJSON;
JsonDocument requestJSON;

bool isSetTempValid(uint8 value)
{
    return value >= 0 && value <= 60;
}

void make_beep()
{
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
}

void make_long_beep()
{
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
}

void make_ok_beep()
{
    for (int i = 0; i < 2; i++)
    {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
    }
}

void handle_getVersion()
{
    Serial.println("Handling GET version API request");
    versionJSON["git_revision"] = GIT_REVISION;
    versionJSON["build_timestamp"] = BUILD_TIMESTAMP;
    String json;
    serializeJson(versionJSON, json);
    Serial.println(json);
    server.send(200, "application/json", json);
}

void handle_getAPI()
{
    Serial.println("Handling GET API request");
    responseJSON["temp"] = round(temp);
    responseJSON["setTemp"] = setTemp;
    responseJSON["state"] = state;
    String json;
    serializeJson(responseJSON, json);
    Serial.println(json);
    server.send(200, "application/json", json);
}

void handle_postAPI()
{
    Serial.println("Handling POST API request");
    make_beep();
    // Get the body of the POST request
    String postBody = server.arg("plain");

    // Print the body of the POST request
    Serial.println("POST body:");
    Serial.println(postBody);

    if (deserializeJson(requestJSON, postBody) == DeserializationError::Ok)
    {
        uint8_t value = requestJSON["temp"];
        if (isSetTempValid(value))
        {
            setTemp = value;
            server.send(200);
            EEPROM.put(EEPROM_ADDRESS, setTemp); // write data to array in ram
            EEPROM.commit();
        }
        else
        {
            server.send(400, "Temp out of range [0,60]");
        }
    }
    else
    {
        server.send(400);
    }
}

void handle_indexHtml()
{
    Serial.println("Sending index.html");
    server.send(200, "text/html", indexHtml);
}

void handle_NotFound()
{
    server.send(404);
}

void processPID()
{

    if (temp <= setTemp)
    {
        state = "Heating";
        digitalWrite(HEATER_PIN, HIGH);
    }
    else
    {
        state = "OFF";
        digitalWrite(HEATER_PIN, LOW);
    }
}

void connectToWifi(bool mandatory = true)
{
    if (WiFi.status() != WL_CONNECTED && (mandatory || lastWifiRetryMs + WIFI_RETRY_INTERVAL < millis()))
    {
        lastWifiRetryMs = millis();
        Serial.print("(re)Connecting to: ");
        Serial.println(ssid);
        // connect to your local wi-fi network
        WiFi.begin(ssid, password);

        // check wi-fi is connected to wi-fi network
        u_int8_t counter = 0;
        while (WiFi.status() != WL_CONNECTED && counter < 30)
        {
            delay(1000);
            make_beep();
            Serial.print(".");
            counter++;
        }
        if (WiFi.status() != WL_CONNECTED && mandatory)
        {
            Serial.print("No wifi connection afrer 30sec. ESP.reset()");
            ESP.reset();
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("");
            Serial.println("WiFi connected..!");
            Serial.print("Got IP: ");
            Serial.println(WiFi.localIP());
            make_ok_beep();
        }
        else
        {
            Serial.println("No WiFi, will try again later");
            make_long_beep();
        }
    }
}

void setup()
{
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(HEATER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(HEATER_PIN, LOW);
    make_beep();
    Serial.begin(115200);

    // start EEPROM
    EEPROM.begin(1);
    EEPROM.get(EEPROM_ADDRESS, setTemp);
    Serial.print("Read setTemp from eeprom: ");
    Serial.println(setTemp);
    if (!isSetTempValid(setTemp))
    {
        Serial.println("Read setTemp is invalid. Set to 0");
        setTemp = 0;
    }

    // Start the DS18B20 sensor
    sensors.begin();

    connectToWifi();
    timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

    server.on("/", handle_indexHtml);
    server.on("/api/temp", HTTP_GET, handle_getAPI);
    server.on("/version", HTTP_GET, handle_getVersion);
    server.onNotFound(handle_NotFound);

    server.begin();
    Serial.println("HTTP server started");

    // Check InfluxDB server connection
    if (influxDBClient.validateConnection())
    {
        Serial.print("Connected to InfluxDB: ");
        Serial.println(influxDBClient.getServerUrl());
    }
    else
    {
        Serial.print("InfluxDB connection failed: ");
        Serial.println(influxDBClient.getLastErrorMessage());
    }
    // Increase buffer to allow caching of failed writes
    influxDBClient.setWriteOptions(WriteOptions().writePrecision(WRITE_PRECISION).batchSize(MAX_BATCH_SIZE).bufferSize(WRITE_BUFFER_SIZE));

    // Add tags to the data point
    measurementPoint.addTag("device", "esp8266");
    measurementPoint.addTag("SSID", WiFi.SSID());

    ArduinoOTA.onStart([]()
                       { Serial.println("Start"); });
    ArduinoOTA.onEnd([]()
                     { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error)
                       {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed"); });
    ArduinoOTA.begin();
    Serial.println("OTA Ready");
}

void loop()
{
    server.handleClient();
    processPID();
    if (lastTempReadingMs + TEMP_CHECK_INTERVAL < millis())
    {
        sensors.requestTemperatures();
        temp = sensors.getTempCByIndex(0);
        Serial.print(temp);
        Serial.println("ºC");
        lastTempReadingMs = millis();

        measurementPoint.clearFields();
        measurementPoint.setTime(time(nullptr));
        measurementPoint.addField("temperature", temp);
        measurementPoint.addField("set_temperature", setTemp);
        measurementPoint.addField("state", state);
        measurementPoint.addField("rssi", WiFi.RSSI());
        Serial.print("Write to buffer/server: ");
        Serial.println(measurementPoint.toLineProtocol());
        influxDBClient.writePoint(measurementPoint);
        if (!influxDBClient.writePoint(measurementPoint))
        {
            Serial.print("InfluxDB write failed: ");
            Serial.println(influxDBClient.getLastErrorMessage());
        }
    }

    connectToWifi(false);
}