#ifndef __PCD_PLATFORM_DRIVER_DT_SYS_
#define __PCD_PLATFORM_DRIVER_DT_SYS_

#include <linux/cdev.h>
#include <linux/fs.h>
#include "platform.h"

int check_permission(int dev_perm, int acc_mode);
loff_t pcd_lseek(struct file *filp, loff_t offset, int whence);
ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
int pcd_open(struct inode *inode, struct file *filp);
int pcd_release(struct inode *inode, struct file *flip);

enum configs_names{
	PCDEVA1X,
	PCDEVB1X,
	PCDEVC1X,
	PCDEVD1X
};

struct device_config {
	int address;
	int com_speed;
	int accuracy;
};


struct pcdev_private_data {
	struct pcdev_platform_data pdata;
	dev_t dev_num;
	char *buffer;
	struct cdev cdev;
};

struct pcdrv_private_data {
	int total_devices;
	dev_t dev_num_base;
	struct class *class_pcd;
	struct device *device_pcd;
};

#endif // __PCD_PLATFORM_DRIVER_DT_SYS_
