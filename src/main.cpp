#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DallasTemperature.h>
#include <OneWire.h>
/* Put your SSID & Password */
#include "credentials.h"
#include "index_html.h"

#define BUZZER_PIN D0
#define DS1820_PIN D1
#define HEATER_PIN D2
#define TEMP_CHECK_INTERVAL 1000
#define TEMP_HYSTERESIS 1

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(DS1820_PIN);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

u_int64_t lastTempReadingMs;
int temp = 0;
int setTemp = 0;
String state = "Idle";

ESP8266WebServer server(80);

void handle_getAPI()
{
    Serial.println("Handling GET API request");
    String json = "{ \"temp\":" + String(temp) + ",\n\"setTemp\":" + String(setTemp) + ",\n\"state\": \"" + state + "\"\n}";
    Serial.println(json);
    server.send(200, "text/html", json);
}

void handle_postAPI()
{
    Serial.println("Handling POST API request");
    // Get the body of the POST request
    String postBody = server.arg("plain");

    // Print the body of the POST request
    Serial.println("POST body:");
    Serial.println(postBody);

    server.send(200, "text/plain", "POST received");
}

void handle_indexHtml()
{
    Serial.println("Sending index.html");
    server.send(200, "text/html", indexHtml);
}

void handle_NotFound()
{
    server.send(404, "text/plain", "Not found");
}

void make_beep()
{
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
}

void processPID()
{
    Serial.println("processPID");
    if (temp + TEMP_HYSTERESIS < setTemp)
    {
        state = "Heating";
        digitalWrite(HEATER_PIN, HIGH);
    }
    else if (temp + TEMP_HYSTERESIS > setTemp)
    {
        state = "Cooling";
        digitalWrite(HEATER_PIN, LOW);
    }
    else
    {
        state = "Idle";
        digitalWrite(HEATER_PIN, LOW);
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

    // Start the DS18B20 sensor
    sensors.begin();

    Serial.println("Connecting to ");
    Serial.println(ssid);
    // connect to your local wi-fi network
    WiFi.begin(ssid, password);

    // check wi-fi is connected to wi-fi network
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected..!");
    Serial.print("Got IP: ");
    Serial.println(WiFi.localIP());
    make_beep();
    delay(500);
    make_beep();

    server.on("/", handle_indexHtml);
    server.on("/api/temp", HTTP_GET, handle_getAPI);
    // server.on("/api/temp", HTTP_POST, handle_postAPI);
    server.onNotFound(handle_NotFound);

    server.begin();
    Serial.println("HTTP server started");
}

void loop()
{
    server.handleClient();
    processPID();
    if (lastTempReadingMs + TEMP_CHECK_INTERVAL < millis())
    {
        sensors.requestTemperatures();
        temp = round(sensors.getTempCByIndex(0));
        Serial.print(temp);
        Serial.println("ºC");
        lastTempReadingMs = millis();
    }
}