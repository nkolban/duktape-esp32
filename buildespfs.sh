#!/bin/bash
#
#  Build an ESPFS image of the file system we are to supply with
#  ESP32-Duktape.  The result will be in build/spiffs.img.
#
set -e
cd filesystem
find . -print | mkespfsimage -c 0 > ../build/espfs.img
# ls *.js | mkespfsimage -c 0 > ../build/espfs.img
cd ..
python $ESP_IDF_PATH/components/esptool_py/esptool/esptool.py \
 --chip esp32 --port "/dev/ttyUSB0" --baud 921600 write_flash --flash_mode "dio" \
 --flash_freq "40m" --flash_size "4MB" --compress 0x360000 build/espfs.img
