PROJECT_NAME := esp32_music_box

ADF_PATH := ./esp-adf
# use the ESP_IDF in the ESP_ADF folder
IDF_PATH := $(ADF_PATH)/esp-idf

# and then use the makefile from ESP_ADF
include $(ADF_PATH)/project.mk