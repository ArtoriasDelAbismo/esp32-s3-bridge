#include <Arduino.h>
#include <USB.h>
#include <WiFi.h>
#include "secrets.h"

bool ledOn = false;

// Serial setup
USBCDC USBSerial;

// Create a server object
WiFiServer server(2323);

// Create an empty 'phone line'
WiFiClient client;

// create an object of type USBCDC (USB serial port). The type is a whole device 

void setup() {
    Serial.begin(115200);
    // prepares de serial port
    USBSerial.begin();
    // Starts the USB hardware
    USB.begin();
    Serial.println("Bridge ready");

    // Start wifi
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while(WiFi.status() != WL_CONNECTED) {
        Serial.println(".");
        delay(500);
    }

    Serial.println(WiFi.localIP());
    server.begin();
}


void loop() {

    if(!client.connected()) {

        client = server.accept();

        if(client) {
            ledOn = !ledOn;
            if (ledOn) {
                rgbLedWrite(48, 0, 0, 20);
            }

            Serial.println("Client connected");
        }
    }

    // while theres bytes to be read
    while (client.available()) 
    {
        // Reads one byte (one typed character)
        int c = client.read();
        USBSerial.write(c);
    }

    while(USBSerial.available()) 
    {
        int c = USBSerial.read();
        client.write(c);
    }
    
}