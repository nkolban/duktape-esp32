#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := esp32-duktape

include $(IDF_PATH)/make/project.mk

ship: images all
	echo "+---------------+"
	echo "| Building ship |"
	echo "+---------------+"	
	mkdir -p build/ship
	cp build/esp32-duktape.bin build/ship
	cp build/partitions_singleapp.bin build/ship
	cp build/bootloader/bootloader.bin build/ship
	cp spiffs.img build/ship
	cp espfs.img build/ship
	cp data/install_binaries.sh build/ship
	cd build/ship; tar -cvzf ../../esp32-duktape-$(shell date "+%Y-%m-%d").tar.gz .
	echo "+---------+"
	echo "| Results |"
	echo "+---------+"	
	echo "Created output: esp32-duktape-$(shell date "+%Y-%m-%d").tar.gz"
	

images:
	echo "+--------------------+"
	echo "| Building espfs.img |"
	echo "+--------------------+"
	cd filesystem; ls *.js | mkespfsimage > ../espfs.img
	echo "+---------------------+"
	echo "| Building spiffs.img |"
	echo "+---------------------+"
	mkspiffs -c filesystem -b 65536 -p 256 -s 524288 spiffs.img
	
duktape_install:
	echo "Installing duktape"
	rm -rf components/duktape
	cd ./components; git clone https://github.com/svaarala/duktape.git
	cp ./data/duktape/component.mk ./components/duktape/
	rm ./components/duktape/examples/debug-trans-socket/duk_trans_socket_windows.c
	python ./components/duktape/tools/configure.py \
		--config-metadata components/duktape/config/ \
		--source-directory components/duktape/src-input \
		--option-file components/duktape/config/examples/low_memory.yaml \
		--option-file data/duktape/ESP32-Duktape.yaml \
		--output-directory components/duktape/src
