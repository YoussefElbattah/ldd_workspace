#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#define DEV_MEM_SIZE 512

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt , __func__
/* pseudo device's memory */
char device_buffer[DEV_MEM_SIZE];

/* This holds the device number */
dev_t device_number;

/* Cdev variable  */
struct cdev pcd_cdev;

int pcd_open(struct inode *inode, struct file *filp){
  	pr_info("open was successful \n");
  	return 0;
}

int pcd_release(struct inode *inode, struct file *filp){
        pr_info("release was successful \n");
	return 0;
}

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence){
        pr_info("lseek requested \n");
	pr_info("current value of file position = %lld \n", filp->f_pos);

	loff_t temp;
	if(whence == SEEK_SET){
		if((offset > DEV_MEM_SIZE) || (offset < 0))
			return -EINVAL;
		filp->f_pos = offset;
	}
	else if(whence == SEEK_CUR){
		temp = filp->f_pos + offset;
		if((temp > DEV_MEM_SIZE) || (temp < 0))
			return -EINVAL;
		filp->f_pos = temp;
	}
	else if(whence == SEEK_END){
		temp = DEV_MEM_SIZE + offset;
                if((temp > DEV_MEM_SIZE) || (temp < 0))
                        return -EINVAL;
                filp->f_pos = temp;
	}
	else{
		return -EINVAL;
	}
	pr_info("updated value of file position = %lld \n", filp->f_pos);

	return filp->f_pos;
}

ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos){
        pr_info("read requested for %zu bytes \n", count);
	pr_info("current value of file position = %lld \n", *f_pos);

	/* Adjust the 'count */
	if((count + *f_pos) > DEV_MEM_SIZE){
		count = DEV_MEM_SIZE - *f_pos;
	}

	/* Copy to user */
	if(copy_to_user(buff, &device_buffer[*f_pos], count)){
		return -EFAULT;	
	}

	/* Update the current file position */
	*f_pos += count;

        pr_info("number of bytes successfully read = %zu \n", count);
	pr_info("updated value of file position = %lld \n", *f_pos);
	
	/* Return number of bytes which have been successfully read */
	return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos){
	pr_info("write requested for %zu bytes \n", count);
	pr_info("current file position = %lld \n", *f_pos);

        /* Adjust the 'count */
        if((count + *f_pos) > DEV_MEM_SIZE){
                count = DEV_MEM_SIZE - *f_pos;
        }
	/* Return no memory if count is equal to zero */
	if(!count){
		pr_err("No space left on the device \n");
		return -ENOMEM;
	}

        /* Copy from user */
        if(copy_from_user(&device_buffer[*f_pos], buff, count)){
                return -EFAULT;
        }

        /* Update the current file position */
        *f_pos += count;

        pr_info("number of bytes successfully written = %zu \n", count);
        pr_info("updated file position = %lld \n", *f_pos);
        
        /* Return number of bytes which have been successfully read */
        return count;


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

struct class *class_pcd;

struct device *device_pcd;

static int __init pcd_init(void){
	int ret;
	/* Dynamically allocate a device number */
	
	ret = alloc_chrdev_region(&device_number,0,1,"pcd_devices");
	
	if(ret < 0){
		pr_err("Alloc chrdev region failed\n");
		goto exit;
	}
	pr_info("Device number <major>:<minor> = %d:%d\n",  MAJOR(device_number), MINOR(device_number));
	/* Initialize the cdev structure with fops */
	cdev_init(&pcd_cdev, &pcd_fops);
	pcd_cdev.owner = THIS_MODULE; // We've put it after the func because inside the 
				      // func all pcd_cdev struct is loaded 0

	/* Register a device (cdev structure) with VFS */
	ret = cdev_add(&pcd_cdev, device_number, 1);	
	if(ret < 0){
		pr_err("Cdev add failed\n");
		goto unreg_chrdev;
	}

	/* create device class under sys/class/ */
	class_pcd = class_create("pcd_class");
	if(IS_ERR(class_pcd)){
		pr_err("Class creation failed\n");
		ret = PTR_ERR(class_pcd);// converts Pointer to Error code, ptr -> int
	       	goto cdev_delete;
	}
	/* populate the sysfs with device information */
	device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");

	if(IS_ERR(device_pcd)){
		pr_err("Device creation failed\n");
		ret = PTR_ERR(device_pcd);
		goto class_destroy;
	}
	pr_info("Module init was successful\n");

class_destroy : 
	class_destroy(class_pcd);
cdev_delete : 
	cdev_del(&pcd_cdev);
unreg_chrdev :
	unregister_chrdev_region(device_number, 1);	
exit :
	pr_err("Module insertion failed \n");
	return ret;
}

static void __exit pcd_exit(void){
	/* Remove a device that was created with device_create() */
	device_destroy(class_pcd, device_number);

	/* Destroy a struct class structure */
	class_destroy(class_pcd);

	/* Remove cdev registration from the kernel VFS */
	cdev_del(&pcd_cdev);

	/* Unregister a range of device numbers */
	unregister_chrdev_region(device_number, 1);
	
	pr_info("Module unload was successful\n");
}

module_init(pcd_init);
module_exit(pcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Youssef ELBATTAH");
MODULE_DESCRIPTION("Pseudo character device driver");
MODULE_INFO(board, "BBB with linux rev_5.168.1");
