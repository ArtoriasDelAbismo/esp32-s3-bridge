#include <Arduino.h>

int counter = 0;
bool ledOn = false;


void setup() {
    Serial.begin(115200);
    Serial.println("Hello from esp32-s3!");
}


void loop() {
    ledOn = !ledOn;
    if (ledOn)
    {
        neopixelWrite(48, 20, 0, 0);
    } else {
        neopixelWrite(48, 0, 0, 0);
    }

    Serial.println("count: ");
    Serial.println(counter++);

    delay(500);
    
}