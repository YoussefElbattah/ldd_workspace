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
#include <linux/delay.h>
#include <linux/slab.h>

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt , __func__

#define NUM_PIN 	7
#define HIGH_VALUE 	1
#define LOW_VALUE 	0
#define MAX_ROW 	2
#define MAX_COLUMN 	16	
#define MIN_ROW		1
#define MIN_COLUMN	MIN_ROW

/*LCD commands */
#define LCD_CMD_4DL_2N_5X8F             0x28
#define LCD_CMD_DON_CURON               0x0E
#define LCD_CMD_INCADD                  0x06
#define LCD_CMD_DIS_CLEAR               0X01
#define LCD_CMD_DIS_RETURN_HOME         0x02
#define LCD_CMD_SHIFT_LEFT		0x18
#define LCD_CMD_SHIFT_RIGHT		0x1C
/*Sets CGRAM address. CGRAM data is sent and received after this setting. */
#define LCD_CMD_SET_CGRAM_ADDRESS                       0x40

/* Sets DDRAM address. DDRAM data is sent and received after this setting. */
#define LCD_CMD_SET_DDRAM_ADDRESS                       0x80

#define DDRAM_SECOND_LINE_BASE_ADDR             (LCD_CMD_SET_DDRAM_ADDRESS | 0x40 )
#define DDRAM_FIRST_LINE_BASE_ADDR              LCD_CMD_SET_DDRAM_ADDRESS

#define IS_DESC_ERR(desc, dev, name)                     \
	do {                                             \
		if (IS_ERR(desc)) {                       \
			if (PTR_ERR(desc) == -ENOENT)     \
				dev_err(dev,               \
					"no GPIO assigned to \"%s\"\n", \
					name);                 \
			return PTR_ERR(desc);               \
		}                                            \
	} while (0)

struct lcdev_private_data 
{
	uint8_t x, y;
	struct gpio_desc *desc_rs, *desc_rw, *desc_en;
	struct gpio_desc *desc_d4, *desc_d5, *desc_d6, *desc_d7;
	struct device *lcd_dev;
};

struct lcdrv_private_data 
{
	struct class *class_gpio;
};


struct lcdrv_private_data lcdrv_data;

void write_4_bits(struct lcdev_private_data *lcdev_data, uint8_t data);
void lcd_set_cursor(struct lcdev_private_data *lcdev_data, uint8_t row, uint8_t column);

void lcd_print_char(struct lcdev_private_data *lcdev_data, uint8_t data)
{

        //RS=1, for user data
        gpiod_set_value(lcdev_data->desc_rs, HIGH_VALUE);

        /*R/nW = 0, for write */
        gpiod_set_value(lcdev_data->desc_rw, LOW_VALUE);

        write_4_bits(lcdev_data, data >> 4); /* higher nibble */
        write_4_bits(lcdev_data, data);      /* lower nibble */

	lcdev_data->x++;

    	if (lcdev_data->x > MAX_COLUMN) {
        	lcdev_data->x = MIN_COLUMN;
		lcdev_data->y = (lcdev_data->y == 1) ? 2 : 1;
        	lcd_set_cursor(lcdev_data, lcdev_data->y, lcdev_data->x);
   	}
}


void lcd_print_message(struct lcdev_private_data *lcdev_data, const char *message, size_t count)
{
	int i;
	for(i = 0; i < count ; i++){
		lcd_print_char(lcdev_data, (uint8_t)message[i]);
	}
}

void lcd_send_command(struct lcdev_private_data *lcdev_data, uint8_t command)
{
        /* RS=0 for LCD command */
        gpiod_set_value(lcdev_data->desc_rs, LOW_VALUE);

        /*R/nW = 0, for write */
        gpiod_set_value(lcdev_data->desc_rw, LOW_VALUE);

        write_4_bits(lcdev_data, command >> 4); /* higher nibble */
        write_4_bits(lcdev_data, command);     /* lower nibble */
	
	switch(command){
		case LCD_CMD_DIS_CLEAR:
			lcdev_data->x = 1;
			lcdev_data->y = 1;
			break;
		case LCD_CMD_DIS_RETURN_HOME:
			lcdev_data->x = 1;
                        lcdev_data->y = 1;
                        break;
		default:

	}

}

void write_4_bits(struct lcdev_private_data *lcdev_data, uint8_t data){
	
	/* 4 bits parallel data write */
        gpiod_set_value(lcdev_data->desc_d4, (data >> 0 ) & 0x1);
        gpiod_set_value(lcdev_data->desc_d5, (data >> 1 ) & 0x1);
        gpiod_set_value(lcdev_data->desc_d6, (data >> 2 ) & 0x1);
        gpiod_set_value(lcdev_data->desc_d7, (data >> 3 ) & 0x1);

	gpiod_set_value(lcdev_data->desc_en, LOW_VALUE);
        udelay(1);
        gpiod_set_value(lcdev_data->desc_en, HIGH_VALUE);
        udelay(1);
        gpiod_set_value(lcdev_data->desc_en, LOW_VALUE);
        udelay(100); /* execution time > 37 micro seconds */
	

}

void lcd_set_cursor(struct lcdev_private_data *lcdev_data, uint8_t row, uint8_t column)
{	
	uint8_t cmd = 0;
	column--;
        switch (row)
        {
                case MIN_ROW:
                        /* Set cursor to 1st row address and add index*/
			cmd = column | DDRAM_FIRST_LINE_BASE_ADDR;
                        lcd_send_command(lcdev_data, cmd);
                break;
                case MAX_ROW:
                        /* Set cursor to 2nd row address and add index*/
			cmd = column |= DDRAM_SECOND_LINE_BASE_ADDR;
                        lcd_send_command(lcdev_data, cmd);
                break;
                default:
                break;
        }
}

void lcd_init(struct lcdev_private_data *lcdev_data){
		
	lcdev_data->x = 1;
	lcdev_data->y = 1;
	mdelay(40);

        /* RS=0 for LCD command */
        gpiod_set_value(lcdev_data->desc_rs,LOW_VALUE);

        /*R/nW = 0, for write */
        gpiod_set_value(lcdev_data->desc_rw,LOW_VALUE);

        write_4_bits(lcdev_data, 0x03);
        mdelay(5);

        write_4_bits(lcdev_data, 0x03);
        udelay(100);

        write_4_bits(lcdev_data, 0x03);
        write_4_bits(lcdev_data, 0x02);

	/*4 bit data mode, 2 lines selection , font size 5x8 */
        lcd_send_command(lcdev_data, LCD_CMD_4DL_2N_5X8F);

        /* Display ON, Cursor ON */
        lcd_send_command(lcdev_data, LCD_CMD_DON_CURON);

        lcd_send_command(lcdev_data, LCD_CMD_DIS_CLEAR);
        /*
         * check page number 24 of datasheet.
         * display clear command execution wait time is around 2ms
         */
        mdelay(2);

        /*Address auto increment*/
        lcd_send_command(lcdev_data, LCD_CMD_INCADD);

        lcd_print_message(lcdev_data, "LCD 16x2 driver", 15);
	
}

ssize_t lcdxy_show(struct device *dev, struct device_attribute *attr, char *buf){

	struct lcdev_private_data *lcdev_data = dev_get_drvdata(dev);
	return sprintf(buf,"{%d,%d}\n", lcdev_data->y, lcdev_data->x);
}



ssize_t lcdxy_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	
	struct lcdev_private_data *lcdev_data = dev_get_drvdata(dev);
	char *comma, temp[32];
	int row, col, ret;
	
	comma = strchr(buf, ',');
	if(comma == NULL){
		pr_err("No comma detected \n");
		return -EINVAL;
	}
	memcpy(temp, buf, comma - buf);
	temp[comma - buf] = '\0';
	ret = kstrtoint(temp, 10, &row);
	if(ret){
		pr_err("1: error while converting str to int : %d\n", ret);
		return ret;
	}
	ret = kstrtoint(comma + 1, 10, &col);
	if(ret){
                pr_err("2: error while converting str to int : %d\n", ret);
                return ret;
        }

        if(col > MAX_COLUMN || col < MIN_COLUMN || row > MAX_ROW || row < MIN_ROW){
		pr_err("Invalid row,col \n");
                return -EINVAL;
	}
        
	lcdev_data->y = row;
	lcdev_data->x = col;
	
	lcd_set_cursor(lcdev_data, lcdev_data->y, lcdev_data->x);

	return count;
}

ssize_t lcdcmd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){

	struct lcdev_private_data *lcdev_data = dev_get_drvdata(dev);
    	
	long value;
	int ret = kstrtol(buf, 0, &value);
	if(ret)
		return ret;

	lcd_send_command(lcdev_data, value);

        return count;
}

ssize_t lcdscroll_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){

	struct lcdev_private_data *lcdev_data = dev_get_drvdata(dev);

	if(sysfs_streq(buf, "right"))
		lcd_send_command(lcdev_data, LCD_CMD_SHIFT_RIGHT);
	else if(sysfs_streq(buf, "left"))
		lcd_send_command(lcdev_data, LCD_CMD_SHIFT_LEFT);
	else
		return -EINVAL;
		
        return count;
}

ssize_t lcdtext_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count){
	
	struct lcdev_private_data *lcdev_data = dev_get_drvdata(dev);

	lcd_print_message(lcdev_data, buf, count);

        return count;
}

/* Create 4 attributes */
static DEVICE_ATTR_WO(lcdcmd);
static DEVICE_ATTR_WO(lcdscroll);
static DEVICE_ATTR_WO(lcdtext);
static DEVICE_ATTR_RW(lcdxy);

static struct attribute *lcd_attrs[] = {
        &dev_attr_lcdcmd.attr,
        &dev_attr_lcdscroll.attr,
	&dev_attr_lcdtext.attr,
	&dev_attr_lcdxy.attr,
        NULL
};

static struct attribute_group lcd_attr_group = 
{
	.attrs = lcd_attrs
};
static const struct attribute_group *lcd_attr_groups[] = {
	&lcd_attr_group,
	NULL

};



int lcd_probe(struct platform_device *pdev){
	
	struct device *dev = &pdev->dev;
	struct lcdev_private_data *lcdev_data;
	
	lcdev_data = devm_kzalloc(dev, sizeof(struct lcdev_private_data), GFP_KERNEL);

    	lcdev_data->desc_rs = devm_gpiod_get(dev, "rs", GPIOD_OUT_LOW);
	IS_DESC_ERR(lcdev_data->desc_rs, dev, "rs");
	
	lcdev_data->desc_rw = devm_gpiod_get(dev, "rw", GPIOD_OUT_LOW);
	IS_DESC_ERR(lcdev_data->desc_rw, dev, "rw");
	
	lcdev_data->desc_en = devm_gpiod_get(dev, "en", GPIOD_OUT_LOW);
	IS_DESC_ERR(lcdev_data->desc_en, dev, "en");

	lcdev_data->desc_d4 = devm_gpiod_get(dev, "d4", GPIOD_OUT_LOW);
	IS_DESC_ERR(lcdev_data->desc_d4, dev, "d4");
	
	lcdev_data->desc_d5 = devm_gpiod_get(dev, "d5", GPIOD_OUT_LOW);
        IS_DESC_ERR(lcdev_data->desc_d5, dev, "d5");

	lcdev_data->desc_d6 = devm_gpiod_get(dev, "d6", GPIOD_OUT_LOW);
	IS_DESC_ERR(lcdev_data->desc_d6, dev, "d6");

	lcdev_data->desc_d7 = devm_gpiod_get(dev, "d7", GPIOD_OUT_LOW);
        IS_DESC_ERR(lcdev_data->desc_d7, dev, "d7");


	/* set platform pdata to remove device when remove callback is called*/
	platform_set_drvdata(pdev, lcdev_data);
	
	/*create device and sys files with the same function */
        lcdev_data->lcd_dev = device_create_with_groups(lcdrv_data.class_gpio, dev, 0, lcdev_data, lcd_attr_groups, dev->of_node->name);
               
        if(IS_ERR(lcdev_data->lcd_dev))
        {
		dev_err(dev, "Error while creating device & sysfs files\n");
                return PTR_ERR(lcdev_data->lcd_dev);
        }          
    
	lcd_init(lcdev_data);

	return 0;

}

int lcd_remove(struct platform_device *pdev){
	
	struct lcdev_private_data *lcdev_data = platform_get_drvdata(pdev);
	lcd_send_command(lcdev_data, LCD_CMD_DIS_CLEAR);
	dev_info(&pdev->dev, "remove called\n");
	device_unregister(lcdev_data->lcd_dev);

	return 0;
}
/* struct to match platform driver to device tree*/
struct of_device_id lcd_device_match[] = {
	{.compatible = "org,bone-lcd"},
	{ }

};

struct platform_driver lcd_platform_driver = {
	.probe = lcd_probe,
	.remove = lcd_remove,
	.driver =  {
		.name = "bone-lcd",
		.of_match_table = of_match_ptr(lcd_device_match)
	}

};
int __init lcd_drv_init(void){

	lcdrv_data.class_gpio = class_create(THIS_MODULE, "bone_lcd");
	if(IS_ERR(lcdrv_data.class_gpio)){
		pr_err("Couldn't create bone_lcd class \n");
		return PTR_ERR(lcdrv_data.class_gpio);
	}
	
	/* Register the platfrom driver */
	platform_driver_register(&lcd_platform_driver);
	
	return 0;
}

void __exit lcd_drv_cleanup(void){

	platform_driver_unregister(&lcd_platform_driver);
	
	class_destroy(lcdrv_data.class_gpio);

}

module_init(lcd_drv_init);
module_exit(lcd_drv_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Youssef ELBATTAH");
MODULE_DESCRIPTION("LCD driver");
MODULE_INFO(board, "BBB with linux rev_5.168.1");
