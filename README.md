# doorbell
Make your cheap doorbell smart

Inspired by https://www.mikrocontroller.net/topic/444994

Big thanks to the guys on mikrocontroller.net, especially Jürgen. I would never have started this project without your work on the SIP part.

**USE AT YOUR OWN RISK!**

# Overview

This repo contains the pcb files as well as the software to build an interface device which makes cheap doorbells smart. Just connect the board between your existing bell button and your chime. Depending on the software configuration the button can afterwards ring your phones (via SIP), publish to an MQTT server or just ring your chime as before.

<p align="center">
    <img src="docs/pcb.png?raw=true">
</p>

I started this project to get a cheap solution that rings my FRITZ!Fons (just ring no crosstalk) whenever someone presses my doorbell's button. In addition i wanted to integrate the doorbell into my Home Assistant installation to be able to disable the bell when i don't want to be disturbed and get notifications via push messages when i'm away.

Under the following links one can find the configuration description and two example setups:
* [Configuration](docs/configuration.md)
* [Example 1](docs/example1.md)
* [Example 2](docs/example2.md)

# Folder structure

* `docs`: Documentation
* `doorbell`: Arduino project for ESP8266
* `pcb`: Schematic and board files

# Thing to do first

1. Get a pcb
    * Supply pcb with a voltage between 8-12V (AC or DC) via pins 1 & 2 of `CN1`
    * Check if 5V and 3.3V are fine
1. Get tools listed below
1. Rename doorbell/config_template.h to doorbell/config.h and adjust to your environment
1. Add `esp8266` boards to Arduino IDE
    * Open Preferences window
    * Enter http://arduino.esp8266.com/stable/package_esp8266com_index.json into Additional Board Manager URLs field. You can add multiple URLs, separating them with commas.
    * Open Boards Manager from Tools > Board menu and find `esp8266` platform.
    * Click install button.
    * Select `NodeMCU 1.0 (ESP-12E Module)` as board
1. Add Libraries to Arduino IDE
    * Open Sketch > Include library -> Manage libraries...
    * Install `PubSubClient` (tested with version 2.8.0)
    * Install `PubSubClientTools` (tested with version 0.6.0)
1. First flashing of `esp8266`
    * Get a cheap USB-UART converter (like the ones with a `CP210x`)
    * Connect the converter to `H1` (RX, TX and GND)
    * Short `H3` (Boot mode)
    * Power up board. If board was powered before reset `esp8266` by shorting `H2`(Reset) for a short time

# Used tools

## Schematic and board
* EasyEDA

## ESP8266
* Arduino IDE 1.8.13
* USB-UART converter
