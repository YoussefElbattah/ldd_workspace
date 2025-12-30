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
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/sysfs.h>
#include "pcd_platform_driver_dt_sys.h"
#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt , __func__

struct device_config dev_conf[] = {
	
	{.address = 0x71, .com_speed = 800, .accuracy = 10},
	{.address = 0xA1, .com_speed = 1000,.accuracy = 20},
	{.address = 0xB1, .com_speed = 100, .accuracy = 1},
	{ },
};


struct pcdrv_private_data pcdrv_data;

/* file operations of the driver */
struct file_operations pcd_fops = {
	.llseek = pcd_lseek,
	.write = pcd_write,
	.read = pcd_read,
	.open = pcd_open,
	.release = pcd_release,
	.owner = THIS_MODULE	
};

ssize_t max_size_show(struct device *dev, struct device_attribute *attr, char *buf){
	/* get access to the device private data 
	struct pcdev_platform_data *pdata = get_platform_data_dt(dev);
        if(IS_ERR(pdata)){
                return PTR_ERR(pdata);
        }*/

	/* sysfs 'dev' is the child from device_create(); private data is on dev->parent */
	/* dev->parent == pdev->dev, which is where probe() stored the driver's private data */
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
	return sprintf(buf, "%d\n", dev_data->pdata.size); 
}
ssize_t max_size_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	long result;
	int ret;
	ret = kstrtol(buf, 10, &result);
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
	dev_data->pdata.size = result;

	dev_data->buffer = krealloc(dev_data->buffer, dev_data->pdata.size, GFP_KERNEL);
	return count ; 
}

ssize_t serial_num_show(struct device *dev, struct device_attribute *attr, char *buf){
	
	struct pcdev_private_data *dev_data = dev_get_drvdata(dev->parent);
        return sprintf(buf, "%s\n", dev_data->pdata.serial_number);
}

/* Create 2 variables struct device_attribute */
static DEVICE_ATTR(max_size, S_IRUGO|S_IWUSR, max_size_show, max_size_store);
static DEVICE_ATTR(serial_num, S_IRUGO, serial_num_show, NULL);

struct attribute *pcd_attrs[] = {
	&dev_attr_max_size.attr,
	&dev_attr_serial_num.attr,
	NULL
};

struct attribute_group pcd_attr_group = {
	.attrs = pcd_attrs
};
struct pcdev_platform_data* get_platform_data_dt(struct device *dev){

	struct device_node *dev_node;
	struct pcdev_platform_data* pdata;
	dev_node = dev->of_node;
	if(!dev_node){
		/* if dev_node is null that means the probe function wasn't called from device tree matching */
		return NULL;
	}
	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if(!pdata){
		dev_err(dev,"No memory to allocate\n");
		return ERR_PTR(-ENOMEM);
	}

	if(of_property_read_string(dev_node, "youssef,device-serial-num", &pdata->serial_number)){
		dev_err(dev, "Missing serial number proprety");
		return ERR_PTR(-EINVAL);
	}

	if(of_property_read_u32(dev_node, "youssef,size", &pdata->size)){
                dev_err(dev, "Missing size proprety");
                return ERR_PTR(-EINVAL);
        }

	if(of_property_read_u32(dev_node, "youssef,perm", &pdata->perm)){
                dev_err(dev, "Missing perm proprety");
                return ERR_PTR(-EINVAL);
        }

	return pdata;
	
}

struct of_device_id youssef_pcdev_dt_match[];


int pcd_sysfs_create_files(struct device *pcd_dev){

	int ret;
#if 0 
	ret = sysfs_create_file(&pcd_dev->kobj, &dev_attr_max_size.attr);
	if(ret < 0){
		return ret;
	}
	return sysfs_create_file(&pcd_dev->kobj, &dev_attr_serial_num.attr);
#endif 
	return sysfs_create_group(&pcd_dev->kobj, &pcd_attr_group);
}
/* Gets called whenever there's a match with other devices*/
int pcd_platform_driver_probe(struct platform_device *pdev){
	
	dev_info(&pdev->dev, "device is detected \n");
	
	int ret;
	struct pcdev_private_data *device_data;
	struct pcdev_platform_data *pdata;
	
	int driver_data;
	const struct of_device_id *match;

	match = of_match_device(of_match_ptr(youssef_pcdev_dt_match), &pdev->dev); 
	
	if(match){
		pdata = get_platform_data_dt(&pdev->dev);
		if(IS_ERR(pdata)){
        		return PTR_ERR(pdata);
	        }
		driver_data = (int)match->data;
	}else{
	/* 1. Get platform data of device, to get it you should return to the file of pcd_device_setup.c and look at the structure platform_device */
                pdata = (struct pcdev_platform_data*)dev_get_platdata(&pdev->dev);
                if(!pdata){
                        dev_err(&pdev->dev, "No platform data exists \n");
                        return -EINVAL;
                }
                driver_data = pdev->id_entry->driver_data;
	}

	/* 2. Dynamically allocate memory for the device private data */
	device_data = devm_kzalloc(&pdev->dev, sizeof(*device_data), GFP_KERNEL);
	if(!device_data){
		dev_err(&pdev->dev, "No memory to allocate\n");
		return -ENOMEM;
	}

	device_data->pdata.size = pdata->size;
	device_data->pdata.serial_number = pdata->serial_number;
	device_data->pdata.perm = pdata->perm;
	
	/* 3. Dynamically allocate memory for the device buffer using size information from the platform data */
	device_data->buffer = devm_kzalloc(&pdev->dev, device_data->pdata.size, GFP_KERNEL);
	if(!device_data->buffer){
               	dev_err(&pdev->dev,"No memory to allocate for the buffer\n");
                return -ENOMEM;
        }

	/* 4. Get the device number */
	device_data->dev_num = pcdrv_data.dev_num_base + pcdrv_data.total_devices;

	/* 5. Do cdev_init and cdev_add for the device */
	cdev_init(&device_data->cdev, &pcd_fops);
	device_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&device_data->cdev, device_data->dev_num, 1);
	if(ret < 0){
		dev_err(&pdev->dev, "Cannot add device\n");
		return ret;
	}

	/* 6. Create device file for the detected platform device */
	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, &pdev->dev, device_data->dev_num, NULL, "pcdevice_%d", pcdrv_data.total_devices);
	if(IS_ERR(pcdrv_data.device_pcd)){
		dev_err(&pdev->dev,"Device file creation failed\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		cdev_del(&device_data->cdev);
	}
	
	ret = pcd_sysfs_create_files(pcdrv_data.device_pcd);

	if(ret < 0){
		device_destroy(pcdrv_data.class_pcd, device_data->dev_num);
		return ret;
	}
	/* save the device private data pointer in the platform device structure */
	dev_set_drvdata(&pdev->dev, device_data);

	/* 7. Error handling */
	dev_info(&pdev->dev, "%s device is created \n", pdev->name);
	dev_info(&pdev->dev, "address : %X | COM_SPEED : %d KHz | Accuracy : +/- %d/100 \n", dev_conf[driver_data].address, 
											     dev_conf[driver_data].com_speed,
											     dev_conf[driver_data].accuracy);
	dev_info(&pdev->dev, "Serial_num : %s | size : %d | permission : 0x%X\n", device_data->pdata.serial_number, 
										  device_data->pdata.size,
										  device_data->pdata.perm);
	dev_info(&pdev->dev, "total of devices is %d\n", ++pcdrv_data.total_devices);
	return 0;
}
/* Gets called whenever devices are being removed */
int pcd_platform_driver_remove(struct platform_device *pdev){

	dev_info(&pdev->dev, "device is removed \n");
	struct pcdev_private_data *pcdev_data = dev_get_drvdata(&pdev->dev);

	/* Remove device that was created with device_create() */
	device_destroy(pcdrv_data.class_pcd, pcdev_data->dev_num);
        
	/* Remove cdev entry from the system */
	cdev_del(&pcdev_data->cdev);
	
	dev_info(&pdev->dev, "total of devices is %d\n", --pcdrv_data.total_devices);
	return 0;
}

struct platform_device_id pcdevs_ids[] = {
	[0] = {.name = "pcdev-A1x", .driver_data = PCDEVA1X},
	[1] = {.name = "pcdev-B1x", .driver_data = PCDEVB1X},
	[2] = {.name = "pcdev-C1x", .driver_data = PCDEVC1X}

};

struct of_device_id youssef_pcdev_dt_match[] = {
	{.compatible = "pcdev-A1x", .data = (void*)PCDEVA1X},
	{.compatible = "pcdev-B1x", .data = (void*)PCDEVB1X},
	{.compatible = "pcdev-C1x", .data = (void*)PCDEVC1X},
	{.compatible = "pcdev-D1x", .data = (void*)PCDEVD1X},
	{}


};
struct platform_driver pcd_platform_driver = {
	.probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.id_table = pcdevs_ids,
	.driver = {
		.name = "pseudo_char_driver", // no longer used by matching function (get a look in platform_match()) when id_table is used
		.of_match_table = of_match_ptr(youssef_pcdev_dt_match),
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
	pcdrv_data.class_pcd = class_create("pcd_class");
        if(IS_ERR(pcdrv_data.class_pcd)){
                pr_err("Class creation failed\n");
                ret = PTR_ERR(pcdrv_data.class_pcd);// converts Pointer to Error code, ptr -> int
                unregister_chrdev_region(pcdrv_data.dev_num_base, MAX_DEVICES);
		return ret;
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
