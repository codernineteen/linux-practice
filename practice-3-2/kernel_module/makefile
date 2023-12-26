obj-m := cas_module.o
KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) module

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
