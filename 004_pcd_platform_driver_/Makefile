#obj-m := pcd_platform_driver.o pcd_device_setup.o

obj-m := pcd_device_setup.o
ARCH=arm 
CROSS_COMPILE=arm-linux-gnueabihf-
KERN_DIR= /home/youssefelbattah/workspace/ldd/source/linux_v5.10.168/
HOST_KERN_DIR= /lib/modules/$(shell uname -r)/build/
DTB_FILE= /home/youssefelbattah/workspace/ldd/source/linux_v5.10.168/arch/arm/boot/dts/am335x-boneblack.dtb
BEAGLE_SSH= debian@192.168.7.2:/home/debian/drivers
all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules

clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) clean

help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) help

host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules
copy-dtb:
	scp $(DTB_FILE) $(BEAGLE_SSH)
copy-drv:
	scp pcd_device_setup.ko $(BEAGLE_SSH)

