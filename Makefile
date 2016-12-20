#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp32-duktape

include $(IDF_PATH)/make/project.mk

ship: images
	echo "+---------------+"
	echo "| Building ship |"
	echo "+---------------+"	
	mkdir -p build/ship
	cp build/esp32-duktape.bin build/ship
	cp build/partitions_singleapp.bin build/ship
	cp build/bootloader/bootloader.bin build/ship
	cp spiffs.img build/ship
	cp espfs.img build/ship
	

images:
	echo "+--------------------+"
	echo "| Building espfs.img |"
	echo "+--------------------+"
	cd spiffs; ls *.js | ~/bin/mkespfsimage > ../espfs.img
	echo "+---------------------+"
	echo "| Building spiffs.img |"
	echo "+---------------------+"
	mkspiffs -c spiffs -b 65536 -p 256 -s 524288 spiffs.img
	

