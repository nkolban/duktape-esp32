#!/bin/bash
#
#  Script to install the ESP32-Duktape images into an ESP32.
#

PORT="/dev/ttyUSB0"
BAUD="115200"

set -e

if [ ! -z "$IDF_PATH" ]
then
	echo "The IDF_PATH environment variable is not set.  This should be"
	echo "set to the directory which is the root of where your ESP-IDF"
	echo "was installed."
	exit 1
fi

python $IDF_PATH/components/esptool_py/esptool/esptool.py \
 --chip esp32 --port "$PORT" --baud $BAUD write_flash --flash_mode "dio" \
 --flash_freq "40m" --flash_size "4MB" --compress \
 0x1000   bootloader.bin \
 0x8000   partitions_singleapp.bin \
 0x10000  esp32-duktape.bin \
 0x180000 spiffs.img \
 0x360000 espfs.img