#!/bin/bash
cd spiffs; ls *.js | ~/bin/mkespfsimage > ../espfs.img
cd ..
python $ESP_IDF_PATH/components/esptool_py/esptool/esptool.py \
 --chip esp32 --port "/dev/ttyUSB0" --baud 921600 write_flash --flash_mode "dio" \
 --flash_freq "40m" --flash_size "4MB" --compress 0x360000 espfs.img
