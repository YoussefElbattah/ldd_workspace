obj-m := lcd_platform_driver.o
ARCH=arm 
CROSS_COMPILE=arm-linux-gnueabihf-
KERN_DIR= /home/youssefelbattah/workspace/ldd/source/linux_v5.10.168/
HOST_KERN_DIR= /lib/modules/$(shell uname -r)/build/
DTB_FILE= /home/youssefelbattah/workspace/ldd/source/linux_v5.10.168/arch/arm/boot/dts/am335x-boneblack.dtb
BEAGLE_SSH= debian@192.168.7.2:/home/debian/drivers
#DRV_FILE= /home/youssefelbattah/workspace/ldd/custom_drivers/005_pcd_platform_driver_dt/pcd_platform_driver_dt.ko

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules

clean:
	make -C $(KERN_DIR) M=$(PWD) clean
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) clean
	rm -rf *.o *.elf
help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) help

host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules
copy-dtb:
	scp $(DTB_FILE) $(BEAGLE_SSH)
copy-files:
	scp *.c *.h $(BEAGLE_SSH)

app: app.o lcd.o bme280.o app.elf

app.o: app.c
                $(CROSS_COMPILE)gcc -c -o $@ $^

lcd.o: lcd.c
                $(CROSS_COMPILE)gcc -c -o $@ $^

bme280.o: bme280.c
                $(CROSS_COMPILE)gcc -c -o $@ $^

app.elf: app.o lcd.o bme280.o
                $(CROSS_COMPILE)gcc -o $@ $^

