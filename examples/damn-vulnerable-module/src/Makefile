obj-m := dvm.o
KDIR = ../../linux-6.6.21

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean