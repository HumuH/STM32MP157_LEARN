#!/bin/sh

make distclean
make stm32mp1_learn_defconfig
make uImage dtbs LOADADDR=0XC2000040 -j4
sync

cp arch/arm/boot/uImage out/
cp arch/arm/boot/dts/stm32mp157d-learn.dtb out/
sync