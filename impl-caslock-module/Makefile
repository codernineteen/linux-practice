#
#SPDX-License-Identifier: GPL-2.0
#
# Makefile for the linked list module
#


obj-m 			+= lockfree_module.o

KERNEL_DIR		:= /lib/modules/$(shell uname -r)/build
PWD			:= $(shell pwd)

default :
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean :
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
