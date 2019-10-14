# ubuntu
sudo apt-get install libssl-dev

# toolchain
gcc-linaro-7.4.1-2019.02-x86_64_aarch64-linux-gnu.tar.xz	Download official website(https://www.linaro.org/downloads)

# Linux
init code：
	https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/snapshot/linux-5.2.tar.gz

build：
	make ginpot_h5_defconfig ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
	make -j4 Image dtbsARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-

Uboot startup parameters:
	setenv bootargs console=ttyS0,115200 earlyprintk root=/dev/mmcblk0p2 rootfstype=ext4 rw rootwait panic=10
	setenv bootcmd 'fatload mmc 0 0x48000000 sun50i-h5-ginpot.dtb;fatload mmc 0 0x46000000 Image;booti 0x46000000 - 0x48000000'
	saveen