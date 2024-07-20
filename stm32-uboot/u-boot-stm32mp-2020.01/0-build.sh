#!/bin/sh


make distclean
make stm32mp15_learn_defconfig
make DEVICE_TREE=stm32mp157d-learn all -j4

cp u-boot.stm32 out/