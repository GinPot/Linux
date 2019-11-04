#!/bin/bash

if [ $# != 1 ]
then
	echo "ERROR,Parameter format:"
	echo "	./build.sh [select]"
	exit
fi

source /ev_toolchain.sh


if [ $1 = "all" ]
then
	make -j4 Image dtbs ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
	cp arch/arm64/boot/Image ./
	cp arch/arm64/boot/dts/allwinner/sun50i-h5-ginpot.dtb ./
	exit
fi

if [ $1 = "dtb" ]
then
	make dtbs ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
	cp arch/arm64/boot/dts/allwinner/sun50i-h5-ginpot.dtb ./
	exit
fi

if [ $1 = "image" ]
then
	make Image ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
	cp arch/arm64/boot/Image ./
	exit
fi

if [ $1 = "modules" ]
then
	make modules ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
	exit
fi

if [ $1 = "menuconfig" ]
then
	make menuconfig ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
	exit
fi

if [ $1 = "clean" ]
then
	make clean ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
	exit
fi

if [ $1 = "defconfig" ]
then
	make ginpot_h5_defconfig ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
	exit
fi