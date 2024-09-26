KERNELDIR := /root/STM32MP157_LEARN/stm32-kernel/linux-5.4.31
CURRENT_PATH := ${shell pwd}
GCC=arm-none-linux-gnueabihf-gcc

############################
obj-m := key_interrupt_driver.o
driver_src := key_interrupt_driver.c
app_target := AppKey
app_src := app_key.c
############################

build: kernel_modules app

kernel_modules:${driver_src}
	${MAKE} -C ${KERNELDIR} M=${CURRENT_PATH} modules
app:${app_src}
	${GCC} $^ -o ${app_target}
clean:
	${MAKE} -C ${KERNELDIR} M=${CURRENT_PATH} clean
	rm ${app_target}