# IoT Wi-Fi Lamp
The goal of this project was to convert two night lamps into syncronized Wi-Fi LED powered lamps.

This was done using two ESP-32 microcontrollers (specifically ESP32-WROOM-32) that come with integrated Wi-Fi, two 24-bit RGB LEDs WS2812b that were programmed in the Arduino IDE using the Espressif's official library - https://github.com/espressif/arduino-esp32.git

The messaging was done through a Pub/Sub pattern using PubNub's service.

Most of the code should be explained in the comments, in case you have any questions, feel free to write me an email: danielrey1@gmail.com
