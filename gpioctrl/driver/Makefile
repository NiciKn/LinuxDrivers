ifneq ($(KERNELRELEASE),)
	obj-m := gpioctrl.o

else
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
endif

clean:
	rm -rf *.ko *.o .*.cmd .tmp_versions Module.symvers
	rm -rf modules.order *.mod.c
