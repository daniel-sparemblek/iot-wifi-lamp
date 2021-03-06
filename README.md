The goal of this project was to convert two night lamps into synchronized Wi-Fi LED-powered lamps.

This was done using two ESP-32 microcontrollers (specifically ESP32-WROOM-32) that come with integrated Wi-Fi, two 24-bit RGB LEDs WS2812b that were programmed in the Arduino IDE using the Espressif's official library - https://github.com/espressif/arduino-esp32.git

The messaging was done through a Pub/Sub pattern using PubNub's service.

Heavily inspired by https://github.com/pblesi/touch_light, but adapted to the European market. Schematics about which pins to solder can be found in his blog post.

These images explain what functionalities the code provides: https://imgur.com/a/UMALs2h
