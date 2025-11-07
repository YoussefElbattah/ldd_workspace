#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include "platform.h"
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt , __func__

enum configs_names{
	PCDEVA1x,
	PCDEVB1x,
	PCDEVC1x,
	PCDEVD1x
};

struct device_config {
	int address;
	int com_speed;
	int accuracy;
};

struct device_config dev_conf[] = {
	
	[0] = {.address = 0x71, .com_speed = 800, .accuracy = 10},
	[1] = {.address = 0xA1, .com_speed = 1000,.accuracy = 20},
	[2] = {.address = 0xB1, .com_speed = 100, .accuracy = 1}
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

struct pcdrv_private_data pcdrv_data;

int check_permission(int dev_perm, int acc_mode)
{

	if(dev_perm == RDWR)
		return 0;
	
	//ensures readonly access
	if( (dev_perm == RDONLY) && ( (acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE) ) )
		return 0;
	
	//ensures writeonly access
	if( (dev_perm == WRONLY) && ( (acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ) ) )
		return 0;

	return -EPERM;

}


loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{

	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdev_data->pdata.size;
	
	loff_t temp;

	pr_info("lseek requested \n");
	pr_info("Current value of the file position = %lld\n",filp->f_pos);

	switch(whence)
	{
		case SEEK_SET:
			if((offset > max_size) || (offset < 0))
				return -EINVAL;
			filp->f_pos = offset;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if((temp > max_size) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = max_size + offset;
			if((temp > max_size) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		default:
			return -EINVAL;
	}
	
	pr_info("New value of the file position = %lld\n",filp->f_pos);

	return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdev_data->pdata.size;

	pr_info("Read requested for %zu bytes \n",count);
	pr_info("Current file position = %lld\n",*f_pos);

	
	/* Adjust the 'count' */
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;

	/*copy to user */
	if(copy_to_user(buff,pcdev_data->buffer+(*f_pos),count)){
		return -EFAULT;
	}

	/*update the current file postion */
	*f_pos += count;

	pr_info("Number of bytes successfully read = %zu\n",count);
	pr_info("Updated file position = %lld\n",*f_pos);

	/*Return number of bytes which have been successfully read */
	return count;

}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdev_data->pdata.size;
	
	pr_info("Write requested for %zu bytes\n",count);
	pr_info("Current file position = %lld\n",*f_pos);

	
	/* Adjust the 'count' */
	if((*f_pos + count) > max_size)
		count = max_size - *f_pos;

	if(!count){
		pr_err("No space left on the device \n");
		return -ENOMEM;
	}

	/*copy from user */
	if(copy_from_user(pcdev_data->buffer+(*f_pos),buff,count)){
		return -EFAULT;
	}

	/*update the current file postion */
	*f_pos += count;

	pr_info("Number of bytes successfully written = %zu\n",count);
	pr_info("Updated file position = %lld\n",*f_pos);

	/*Return number of bytes which have been successfully written */
	return count;

}



int pcd_open(struct inode *inode, struct file *filp)
{

	int ret;

	int minor_n;
	
	struct pcdev_private_data *pcdev_data;

	/*find out on which device file open was attempted by the user space */

	minor_n = MINOR(inode->i_rdev);
	pr_info("minor access = %d\n",minor_n);

	/*get device's private data structure */
	pcdev_data = container_of(inode->i_cdev,struct pcdev_private_data,cdev);

	/*to supply device private data to other methods of the driver */
	filp->private_data = pcdev_data;
		
	/*check permission */
	ret = check_permission(pcdev_data->pdata.perm,filp->f_mode);

	(!ret)?pr_info("open was successful\n"):pr_info("open was unsuccessful\n");

	return ret;
}

int pcd_release(struct inode *inode, struct file *flip)
{
	pr_info("release was successful\n");

	return 0;
}


/* file operations of the driver */
struct file_operations pcd_fops = {
	.llseek = pcd_lseek,
	.write = pcd_write,
	.read = pcd_read,
	.open = pcd_open,
	.release = pcd_release,
	.owner = THIS_MODULE	
};

/* Gets called whenever there's a match with other devices*/
int pcd_platform_driver_probe(struct platform_device *pdev){
	
	int ret;
	struct pcdev_private_data *device_data;
	struct pcdev_platform_data *pdata;
	
	/* 1. Get platform data of device, to get it you should return to the file of pcd_device_setup.c and look at the structure platform_device */
	pdata = (struct pcdev_platform_data*)dev_get_platdata(&pdev->dev);
	if(!pdata){
		pr_err("No platform data exists \n");
		return -EINVAL;
	}	

	/* 2. Dynamically allocate memory for the device private data */
	device_data = devm_kzalloc(&pdev->dev, sizeof(*device_data), GFP_KERNEL);
	if(!device_data){
		pr_err("No memory to allocate\n");
		return -ENOMEM;
	}

	device_data->pdata.size = pdata->size;
	device_data->pdata.serial_number = pdata->serial_number;
	device_data->pdata.perm = pdata->perm;
	
	/* 3. Dynamically allocate memory for the device buffer using size information from the platform data */
	device_data->buffer = devm_kzalloc(&pdev->dev, device_data->pdata.size, GFP_KERNEL);
	if(!device_data->buffer){
                pr_err("No memory to allocate for the buffer\n");
                return -ENOMEM;
        }

	/* 4. Get the device number */
	device_data->dev_num = pcdrv_data.dev_num_base + pdev->id;

	/* 5. Do cdev_init and cdev_add for the device */
	cdev_init(&device_data->cdev, &pcd_fops);
	device_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&device_data->cdev, device_data->dev_num, 0);
	if(ret < 0){
		pr_err("Cannot add device\n");
		return ret;
	}

	/* 6. Create device file for the detected platform device */
	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, device_data->dev_num, NULL, "pcdevice_%d", pdev->id);
	if(IS_ERR(pcdrv_data.device_pcd)){
		pr_err("Device file creation failed\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		cdev_del(&device_data->cdev);
	}
	
	/* save the device private data pointer in the platform device structure */
	dev_set_drvdata(&pdev->dev, device_data);

	/* 7. Error handling */
	pr_info("%s device is created \n", pdev->name);
	pr_info("address : %X\n", dev_conf[pdev->id_entry->driver_data].address);
	pr_info("COM_SPEED : %d KHz\n", dev_conf[pdev->id_entry->driver_data].com_speed);
	pr_info("Accuracy : +/- 1/%d \n", dev_conf[pdev->id_entry->driver_data].accuracy);
	pr_info("total of devices is %d\n", ++pcdrv_data.total_devices);
		
	return 0;
}
/* Gets called whenever devices are being removed */
int pcd_platform_driver_remove(struct platform_device *pdev){
	
	struct pcdev_private_data *pcdev_data = dev_get_drvdata(&pdev->dev);

	/* Remove device that was created with device_create() */
	device_destroy(pcdrv_data.class_pcd, pcdev_data->dev_num);
        
	/* Remove cdev entry from the system */
	cdev_del(&pcdev_data->cdev);
	
	pr_info("A device with id %d is removed\n", pdev->id);
	pr_info("total of devices is %d\n", --pcdrv_data.total_devices);
	return 0;
}

struct platform_device_id pcdevs_ids[] = {
	[0] = {.name = "pcdev-A1x", .driver_data = PCDEVA1x},
	[1] = {.name = "pcdev-B1x", .driver_data = PCDEVB1x},
	[2] = {.name = "pcdev-C1x", .driver_data = PCDEVC1x}

};
struct platform_driver pcd_platform_driver = {
	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.id_table = pcdevs_ids,
	.driver = {
		.name = "pseudo_char_driver" // no longer used by matching function (get a look in platform_match()) when id_table is used
	}

};

#define MAX_DEVICES 10
static int __init pcd_platform_driver_init(void){
	
	int ret;
	/* Allocates a range of char device number */
	ret = alloc_chrdev_region(&pcdrv_data.dev_num_base, 0, MAX_DEVICES,"pcd_devices");
	if(ret < 0){
		pr_err("Alloc region failed\n");
		return ret;
	}	
	
	/* create device class under sys/class/ */
	pcdrv_data.class_pcd = class_create(THIS_MODULE,"pcd_class");
        if(IS_ERR(pcdrv_data.class_pcd)){
                pr_err("Class creation failed\n");
                ret = PTR_ERR(pcdrv_data.class_pcd);// converts Pointer to Error code, ptr -> int
                unregister_chrdev_region(pcdrv_data.dev_num_base, MAX_DEVICES);
        }

	/* Register a driver for platform-level devices */
	platform_driver_register(&pcd_platform_driver);

	pr_info("Driver load was successful\n");
	
	return 0;
}

static void __exit pcd_platform_driver_exit(void){
	
	/* Unregister a driver */
	platform_driver_unregister(&pcd_platform_driver);

	/* destroy device class under sys/class/ */
	class_destroy(pcdrv_data.class_pcd);

	/* Unregister a range of device numbers */
	unregister_chrdev_region(pcdrv_data.dev_num_base, MAX_DEVICES);	
	
	pr_info("Driver unload was successful\n");
}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Youssef ELBATTAH");
MODULE_DESCRIPTION("Pseudo Platform driver");
MODULE_INFO(board, "BBB with linux rev_5.168.1");
