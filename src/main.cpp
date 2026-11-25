#include <Arduino.h>
#include <USB.h>

bool ledOn = false;
// create an object of type USBCDC (USB serial port). The type is a whole device 
USBCDC USBSerial;

void setup() {
    Serial.begin(115200);
    // prepares de serial port
    USBSerial.begin();
    // Starts the USB hardware
    USB.begin();
    Serial.println("Bridge ready");
}


void loop() {
    // while theres bytes to be read
    while (USBSerial.available()) 
    {
        ledOn = !ledOn;
        if (ledOn) {
            rgbLedWrite(48, 0, 20, 0);
        } else {
            rgbLedWrite(48, 0, 0, 0);
        }

        // Reads one byte (one typed character)
        int c = USBSerial.read();
        // Sends one byte back out
        USBSerial.write(c);
        // Sends it to the debug port
        Serial.write(c);
    }
    
}