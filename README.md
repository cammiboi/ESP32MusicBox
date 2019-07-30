# ESP32MusicBox
Internet-connected music player

## Hardware

## Software
This is based on the ESP-ADF audio framework for the ESP32.

### Step 1- Get ESP_ADF
Use the following git commands to get a copy of ESP-ADF. This project uses the following version.
```
git clone https://github.com/espressif/esp-adf.git esp-adf-v2.0-beta1
cd esp-adf-v2.0-beta1/
git checkout v2.0-beta1
git submodule update --init --recursive
```
### Step 2- Install the ESP Toolchain
Follow the instructions here depending if you have Mac, Windows, or Lunix
https://docs.espressif.com/projects/esp-idf/en/stable/get-started/index.html#setup-toolchain
