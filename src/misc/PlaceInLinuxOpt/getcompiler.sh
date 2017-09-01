#/bin/bash
sudo mkdir /opt/Espressif
sudo chmod 777 /opt/Espressif
cd /opt/Espressif

sudo apt-get update
sudo apt-get upgrade
sudo apt-get install git autoconf build-essential gperf bison flex texinfo libtool libncurses5-dev wget gawk libc6-dev-i386 python-serial libexpat-dev build-essential help2man libtool-bin python python-dev -y

git clone https://github.com/espressif/crosstool-NG.git
cd crosstool-NG; git fetch; cd ..;

cd crosstool-NG
./bootstrap && ./configure --prefix=`pwd` && make && make install
./ct-ng xtensa-lx106-elf
./ct-ng build
PATH=$PWD/builds/xtensa-lx106-elf/bin:$PATH
