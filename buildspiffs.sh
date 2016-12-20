#!/bin/bash
set -e
mkspiffs -c spiffs -b 65536 -p 256 -s 524288 spiffs.img
python $ESP_IDF_PATH/components/esptool_py/esptool/esptool.py \
 --chip esp32 --port "/dev/ttyUSB0" --baud 921600 write_flash --flash_mode "dio" \
 --flash_freq "40m" --flash_size "2MB" --compress 0x180000 spiffs.img


