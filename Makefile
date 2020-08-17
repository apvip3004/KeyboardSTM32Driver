obj-m += button_driver.o
KERNEL_SRC = /home/adam/Tools/rpi3/kernel-source

all:
	make -C $(KERNEL_SRC) M=$(PWD) modules

clean:
	make -C $(KERNEL_SRC) M=$(PWD) clean
