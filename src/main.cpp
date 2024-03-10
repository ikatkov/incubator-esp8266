#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DallasTemperature.h>
#include <OneWire.h>
/* Put your SSID & Password */
#include "credentials.h"

#define BUZZER_PIN D0
#define TEMP_PIN D1
#define LED_PIN D2
#define TEMP_CHECK_INTERVAL 1000

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(TEMP_PIN);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

u_int64_t lastTempReadingMs;
int temp = 0;
int setTemp = 0;
String state = "Idle";

ESP8266WebServer server(80);

const String indexHtml = R"=====(
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta name="iframeUrl" content="https://incubator.katkovonline.com">
<title>Incubator</title>
<style>
    body, html {
        margin: 0;
        padding: 0;
        width: 100%;
        height: 100%;
        overflow: hidden;
    }
    iframe {
        width: 100%;
        height: 100%;
        border: none; /* remove iframe border */
    }
</style>
</head>
<body>

<iframe id="myIframe" src="" frameborder="0"></iframe>

<script>
    // Add an event listener for the window load event
    window.addEventListener('load', function() {
        // Dispatch an event to itself with data 'null' upon loading
        window.dispatchEvent(new CustomEvent('message', { detail: 'null' }));
    });

    window.addEventListener('message', receiveMessage, false);

    function receiveMessage(event) {

        if(!event.data || event.data === 'null') {
            console.log("READ request");
            // Make GET request to retrieve temperature data
            fetch('/api/temp')
            .then(response => {
                if (!response.ok) {
                    throw new Error('Failed to retrieve temperature data');
                }
                return response.json();
            })
            .then(data => {
                console.log("Received data:");
                console.log(data);
                // Parse the received JSON
                const temp = data.temp;
                const setTemp = data.setTemp;
                const state = data.state;

                // Read iframe URL from meta tag
                const iframeUrl = document.querySelector('meta[name="iframeUrl"]').content;

                // Construct the iframe URL with the parsed values
                const fullUrl = `${iframeUrl}/?temp=${temp}&setTemp=${setTemp}&state=${state}`;

                // Set the iframe src attribute
                document.getElementById('myIframe').src = fullUrl;
            })
            .catch(error => {
                console.error('Error:', error);
            });
        }
        else
        {
            console.log('WRITE request');
            // Extract inputVal from the received message
            const inputVal = event.data.inputVal;

            // Make API call
            fetch('/api/temp', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    temperature: inputVal
                })
            })
            .then(response => {
                // Handle the response from the API
                if (response.ok) {
                    console.log('Temperature set successfully');
                } else {
                    console.error('Failed to set temperature');
                }
                // Get reference to the iframe element
                var iframe = document.getElementById('myIframe');

                // Reload the iframe
                // iframe.src = iframe.src;
                window.dispatchEvent(new CustomEvent('message', { detail: 'null' }));
                console.log('refreshed');
            })
            .catch(error => {
                console.error('Error:', error);
            });
        }
    }
</script>

</body>
</html>

)=====";

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

void setup()
{
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
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
    if (lastTempReadingMs + TEMP_CHECK_INTERVAL < millis())
    {
        sensors.requestTemperatures();
        temp = round(sensors.getTempCByIndex(0));
        Serial.print(temp);
        Serial.println("ºC");
    }
}