#makefile for XR17x15y PCI UARTs for Linux 2.6.x Kernel Fedora Core 2

CC := powerpc-linux-gcc
LD := powerpc-linux-ld

KERNEL_SRC = /root/linux-2.6.25

all: build
obj-m += pcifpga.o
build:
	$(MAKE) -C $(KERNEL_SRC) SUBDIRS=$(PWD) modules
clean:
	rm -f *~ *.o
	rm -f *~ *.ko module.* Module.* *.mod.* modules.*
