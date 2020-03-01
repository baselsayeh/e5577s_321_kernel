#!/bin/bash

if [[ $# -ne 3 ]] ; then
	echo 'Warning! using the default kernel'
	KIMAGE_NAME="kernel.img-zImage"
	RAMDISK_NAME="kernel.img-ramdisk.gz"
	SECOND_NAME="kernel.img-second"
else
	KIMAGE_NAME="$1"
	RAMDISK_NAME="$2"
	KIMAGE_NAME="$3"
fi

rm -f bootimg_mod.bin KERNEL.img

mkbootimg --kernel "$KIMAGE_NAME" --ramdisk "$RAMDISK_NAME" --second "$SECOND_NAME" --cmdline "root=/dev/ram0 rw console=ttyAMA0,115200 console=uw_tty0,115200 rdinit=/init loglevel=5 mem=0x3400000" --board "" --base 0x54b08000 --pagesize 4096 --kernel_offset 0x00008000 --ramdisk_offset 0x01000000 --second_offset 0x00f00000 --tags_offset 0x00000100 --header_version 0 --hash sha1 -o bootimg_mod.bin

cat kernel_header bootimg_mod.bin > KERNEL.img

rm -f bootimg_mod.img

