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
Follow the instructions below depending if you have Mac, Windows, or Lunix:

https://docs.espressif.com/projects/esp-idf/en/stable/get-started/index.html#setup-toolchain

### Step 3- Setup path to ESP_ADF
Follow the instructions below, but instead of IDF_PATH, define ADF_PATH to the location you cloned the git folder to in step 1. This will allow the toolchain to find ESP_ADF, and ESP_IDF (which is within the ESP_ADF folder).

https://docs.espressif.com/projects/esp-idf/en/stable/get-started/add-idf_path-to-profile.html#add-idf-path-to-profile-windows
