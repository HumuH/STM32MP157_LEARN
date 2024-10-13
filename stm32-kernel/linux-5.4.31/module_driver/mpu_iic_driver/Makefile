KERNELDIR := /root/STM32MP157_LEARN/stm32-kernel/linux-5.4.31
CURRENT_PATH := ${shell pwd}
GCC=arm-none-linux-gnueabihf-gcc

############################
MODULE_NAME=mpu
obj-m := ${MODULE_NAME}_driver.o
driver_src := ${MODULE_NAME}_driver.c
app_target := App${MODULE_NAME}
app_src := app_${MODULE_NAME}.c
############################

build: kernel_modules app

kernel_modules:${driver_src}
	${MAKE} -C ${KERNELDIR} M=${CURRENT_PATH} modules
app:${app_src}
	${GCC} $^ -o ${app_target}
clean:
	${MAKE} -C ${KERNELDIR} M=${CURRENT_PATH} clean
	rm ${app_target}