#!/bin/bash



if [ $# != 1 ]
then
	echo "ERROR,Parameter format:"
	echo "	./build.sh [select]"
	exit
fi

BASE=$(pwd)
OPTION="-C $PWD ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- O=$PWD/out"
source /ev_toolchain.sh


if [ $1 = "all" ]
then
	make -j4 Image dtbs $OPTION || exit
	cp out/arch/arm64/boot/Image ./out
	cp out/arch/arm64/boot/dts/allwinner/sun50i-h5-ginpot.dtb ./out
	exit
fi

if [ $1 = "dtb" ]
then
	make dtbs $OPTION || exit
	cp arch/arm64/boot/dts/allwinner/sun50i-h5-ginpot.dtb ./
	exit
fi

if [ $1 = "image" ]
then
	make Image $OPTION || exit
	cp arch/arm64/boot/Image ./
	exit
fi

if [ $1 = "modules" ]
then
	make modules $OPTION
	exit
fi

if [ $1 = "menuconfig" ]
then
	make menuconfig $OPTION
	exit
fi

if [ $1 = "clean" ]
then
	make clean $OPTION
	exit
fi

if [ $1 = "defconfig" ]
then
	make ginpot_h5_defconfig $OPTION
	exit
fi

if [ $1 = "mrproper" ]
then
	make mrproper $OPTION
	exit
fi
