# E5577s_321 kernel

## Compilation of the kernel
Refer to kernel/README_Kernel.txt

## Instalation of the kernel
Firstly make sure that you have an extracted bootimg from your device (you can use [osm0sis's mkbootimg](https://github.com/osm0sis/mkbootimg))

After compilation, use mk_bootimg.sh with

param 1: your kernel image

param 2: your extracted ramdisk image

param 3: your extracted second image

then extract your device firmware using [forth32's balong flash](https://github.com/forth32/balongflash) parameter `e`,

Then rename and replace the kernel image (The one that contains `Kernel_R11`), the flash using `balong_flash -n .`


