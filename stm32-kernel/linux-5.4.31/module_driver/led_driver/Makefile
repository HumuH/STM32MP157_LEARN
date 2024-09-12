KERNELDIR := /root/STM32MP157_LEARN/stm32-kernel/linux-5.4.31
CURRENT_PATH := ${shell pwd}
obj-m := led_driver.o

build: kernel_modules

kernel_modules:
	${MAKE} -C ${KERNELDIR} M=${CURRENT_PATH} modules
	arm-none-linux-gnueabihf-gcc app_led.c -o AppLed
clean:
	${MAKE} -C ${KERNELDIR} M=${CURRENT_PATH} clean
	rm AppLed