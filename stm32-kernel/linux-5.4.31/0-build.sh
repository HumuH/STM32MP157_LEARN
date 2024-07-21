#!/bin/sh

make distclean
make stm32mp1_learn_defconfig
make uImage dtbs LOADADDR=0XC2000040 -j4