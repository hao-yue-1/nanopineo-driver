KERNELDIR := /usr/src/linux-headers-4.14.111/
CURRENT_PATH := $(shell pwd)
obj-m := ledy.o

build: kernel_modules

kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
