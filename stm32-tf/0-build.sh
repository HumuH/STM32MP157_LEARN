#!/bin/sh
cd tf-a-stm32mp-2.2.r1/
make -f ../Makefile.sdk -j4 all
cd -
cp build/trusted/tf-a-stm32mp157d-learn.stm32 out/