#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/sysfs.h>
#include <linux/gpio/consumer.h>
#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt , __func__

struct gpiodev_private_data 
{
	char label[20];
	struct gpio_desc *desc;
};

struct gpiodrv_private_data 
{
	int total_devices;
	struct class *class_gpio;
	struct device **dev;
};


struct gpiodrv_private_data gpiodrv_data;

ssize_t direction_show(struct device *dev, struct device_attribute *attr, char *buf){

	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);

	int status = gpiod_get_direction(dev_data->desc);
	if(status < 0)
		return status;
	if(status == 0){
		memcpy(buf, "out", 4);
	}else 
		memcpy(buf, "in", 3);
	
	return 0;
}

ssize_t value_show(struct device *dev, struct device_attribute *attr, char *buf){
	
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);

        int value = gpiod_get_value(dev_data->desc);
        if(value < 0)
                return value;

        return sprintf(buf, "%d\n", value);
}

ssize_t label_show(struct device *dev, struct device_attribute *attr, char *buf){
	
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);

        return sprintf(buf, "%s\n", dev_data->label);
}

ssize_t direction_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){

	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
        int ret;

	if(sysfs_streq(buf, "out"))
	{
		ret = gpiod_direction_output(dev_data->desc, 0);
	}else if(sysfs_streq(buf, "in"))
	{
		ret = gpiod_direction_input(dev_data->desc);
	}else {
		ret = -EINVAL;
		dev_err(dev, "direction is invalid %s\n", buf);
	}

	return ret ? : count;
}

ssize_t value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	
	struct gpiodev_private_data *dev_data = dev_get_drvdata(dev);
	int ret;
	long value;
	
	ret = kstrtol(buf, 0, &value);
	if(ret)
		return ret;
	if(((value <= 0) && (value >= 1)))
		return -EINVAL;

	gpiod_set_value(dev_data->desc, value);

	return count;
}

/* Create 3 attributes */
static DEVICE_ATTR_RW(direction);
static DEVICE_ATTR_RW(value);
static DEVICE_ATTR_RO(label);

static struct attribute *gpio_attrs[] = {
        &dev_attr_direction.attr,
        &dev_attr_value.attr,
	&dev_attr_label.attr,
        NULL
};

static struct attribute_group gpio_attr_group = 
{
	.attrs = gpio_attrs
};

static const struct attribute_group *gpio_attr_groups[] = {
	&gpio_attr_group,
	NULL

};
int gpiosysfs_probe(struct platform_device *pdev){
	
	/* parent device node*/
	struct device *dev = &pdev->dev;
	struct device_node *parent = pdev->dev.of_node;
	struct device_node *child = NULL;
	const char *label_name;
	struct gpiodev_private_data *gpiodev_data;
	int ret;
	int i = 0;
	gpiodrv_data.total_devices = of_get_child_count(parent);
	
	if(gpiodrv_data.total_devices == 0){
		dev_warn(dev, "No child node found\n");
		return -EINVAL;
	}
	
	dev_info(dev, "Total of device found : %d\n", gpiodrv_data.total_devices);
	gpiodrv_data.dev = devm_kzalloc(dev, sizeof(struct device *) * gpiodrv_data.total_devices, GFP_KERNEL);
	
	for_each_available_child_of_node(parent, child)
	{
		gpiodev_data = devm_kzalloc(dev, sizeof(struct gpiodev_private_data), GFP_KERNEL);
		if(!gpiodev_data){
			dev_err(dev,"Couldn't allocate memory for gpio private data\n");
			return -ENOMEM;
		}
	
		if(of_property_read_string(child, "label", &label_name))
		{
			dev_warn(dev, "Couldn't read property label\n");
			snprintf(gpiodev_data->label, sizeof(gpiodev_data->label), "unkgpio%d", i);
		}else{
			strcpy(gpiodev_data->label, label_name);
			dev_info(dev, "GPIO label : %s\n", gpiodev_data->label);
		}
		/* gpiod_get() can't read GPIOs in DT child nodes → use fwnode_get_gpiod_from_child() instead */
		gpiodev_data->desc =  devm_fwnode_get_gpiod_from_child(dev, "bone", &child->fwnode, GPIOD_ASIS, gpiodev_data->label);
	
		if(IS_ERR(gpiodev_data->desc))
		{
			ret = PTR_ERR(gpiodev_data->desc);
			if(ret == -ENOENT)
				dev_err(dev,"No GPIO has been assigned to the requested function and/or index \n");
			return ret;
		}
		
		/* set the gpio direction to output */
		gpiod_direction_output(gpiodev_data->desc, 0);
		if(ret)
		{
			dev_err(dev, "gpio direction set failed\n");
			return ret;
		}

		/*create device and sys files with the same function */
		gpiodrv_data.dev[i] = device_create_with_groups(gpiodrv_data.class_gpio, dev, 0, gpiodev_data, gpio_attr_groups, gpiodev_data->label);
		
		if(IS_ERR(gpiodrv_data.dev[i]))
		{
			dev_err(dev, "Error while creating device & sysfs files\n");
			return PTR_ERR(gpiodrv_data.dev[i]);
		}		
		i++;
	}
	return 0;

}

int gpiosysfs_remove(struct platform_device *pdev){
	
	int i = 0;
	dev_info(&pdev->dev, "remove called\n");
	for(i = 0 ; i < gpiodrv_data.total_devices ; i++){
		device_unregister(gpiodrv_data.dev[i]);
	}

	return 0;
}
/* struct to match platform driver to device tree*/
struct of_device_id gpio_device_match[] = {
	{.compatible = "org,bone-gpio-sysfs"},
	{ }

};

struct platform_driver gpio_platform_driver = {
	.probe = gpiosysfs_probe,
	.remove = gpiosysfs_remove,
	.driver =  {
		.name = "bone-gpio-sysfs",
		.of_match_table = of_match_ptr(gpio_device_match)
	}

};
int __init gpio_sysfs_init(void){

	gpiodrv_data.class_gpio = class_create(THIS_MODULE, "bone_gpios");
	if(IS_ERR(gpiodrv_data.class_gpio)){
		pr_err("Couldn't create bone_gpios class \n");
		return PTR_ERR(gpiodrv_data.class_gpio);
	}
	
	/* Register the platfrom driver */
	platform_driver_register(&gpio_platform_driver);
	
	return 0;
}

void __exit gpio_sysfs_cleanup(void){

	platform_driver_unregister(&gpio_platform_driver);
	
	class_destroy(gpiodrv_data.class_gpio);

}

module_init(gpio_sysfs_init);
module_exit(gpio_sysfs_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Youssef ELBATTAH");
MODULE_DESCRIPTION("GPIO sysfs driver");
MODULE_INFO(board, "BBB with linux rev_5.168.1");
