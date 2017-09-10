#!/bin/bash
boot=new
app=1
spi_speed=40
spi_mode=DIO
spi_size_map=4

touch user/user_main.c

echo ""
echo "start..."
echo ""

export PATH=/opt/Espressif/crosstool-NG/builds/xtensa-lx106-elf/bin:$PATH

make clean
make COMPILE=gcc BOOT=$boot APP=$app SPI_SPEED=$spi_speed SPI_MODE=$spi_mode SPI_SIZE_MAP=$spi_size_map
rm ./firmware/*
mkdir ./firmware

# cp ../bin/boot_v1.6.bin					./firmware/0x00000.bin
# cp ../bin/esp_init_data_default.bin		./firmware/0x3fc000.bin
# cp ../bin/blank.bin						./firmware/0xfe000.bin
# cp ../bin/blank.bin						./firmware/0x3fe000.bin

#cp ../bin/upgrade/user1.1024.new.2.bin	./firmware/0x01000.bin
#cp ../bin/upgrade/user1.2048.new.3.bin	./firmware/0x01000.bin
cp ../bin/upgrade/user1.4096.new.4.bin	./firmware/0x01000.bin

#mv ../bin/eagle.flash.bin 				./firmware/0x00000.bin
#mv ../bin/eagle.irom0text.bin 			./firmware/0x10000.bin
