#!/bin/bash
#
#  Script to install the ESP32-Duktape images into an ESP32.
#
#  For faster transfers that *may* be less reliable, change the baud
# rate to 921600.
#

PORT="/dev/ttyUSB0"
BAUD="115200"

set -e

if [ ! -z "$IDF_PATH" ]
then
	ESPTOOL=$IDF_PATH/components/esptool_py/esptool/esptool.py
fi


if [ -z "$ESPTOOL" ]
then
	echo "Could not find esptool.  Make sure that IDF_PATH is set"
	echo "to the root of your ESP-IDF install"
	exit 1
fi

python $ESPTOOL  \
 --chip esp32 --port "$PORT" --baud $BAUD  write_flash --flash_mode "dio" \
 --flash_freq "40m" --flash_size "4MB" --compress \
 0x1000   bootloader.bin \
 0x8000   partitions_singleapp.bin \
 0x10000  esp32-duktape.bin \
 0x180000 spiffs.img \
 0x300000 espfs.img
  
