#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

#define MEM_SIZE_MAX_PCDEV1 512
#define MEM_SIZE_MAX_PCDEV2 1024
#define MEM_SIZE_MAX_PCDEV3 512
#define MEM_SIZE_MAX_PCDEV4 1024


#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR 0x11


#define NO_DEVICES	4
#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt , __func__
/* pseudo device's memory */
char device_buffer_pcdev1[MEM_SIZE_MAX_PCDEV1];
char device_buffer_pcdev2[MEM_SIZE_MAX_PCDEV2];
char device_buffer_pcdev3[MEM_SIZE_MAX_PCDEV3];
char device_buffer_pcdev4[MEM_SIZE_MAX_PCDEV4];

/* Device private data structure */
struct pcdev_private_data 
{
	char *buffer;
	unsigned size;
	const char *serial_number;
	int perm;
	struct cdev cdev;
};

/* This structure represents driver private data */
struct pcdrv_private_data 
{
	int total_devices;
	dev_t device_number;
	struct class *class_pcd;
	struct device *device_pcd;
	struct pcdev_private_data pcdev_data[NO_DEVICES];
};


struct pcdrv_private_data pcdrv_data = {
	.total_devices = NO_DEVICES,
	.pcdev_data = {
		[0] = {
			.buffer = device_buffer_pcdev1,
			.size = MEM_SIZE_MAX_PCDEV1,
			.serial_number = "PCDEV1XYZ123",
			.perm = RDONLY /* RDONLY */
	
		},
		[1] = {
			.buffer = device_buffer_pcdev2,
                        .size = MEM_SIZE_MAX_PCDEV2,
                        .serial_number = "PCDEV2XYZ123",
                        .perm = WRONLY /* WRONLY */
		},
		[2] = {
                        .buffer = device_buffer_pcdev3,
                        .size = MEM_SIZE_MAX_PCDEV3,
                        .serial_number = "PCDEV3XYZ123",
                        .perm = RDWR /* RDWR */
                },
		[3] = {
                        .buffer = device_buffer_pcdev4,
                        .size = MEM_SIZE_MAX_PCDEV4,
                        .serial_number = "PCDEV4XYZ123",
                        .perm = RDWR /* RDWR */
                }
	
	}


};

int check_permission(int dev_perm, int access_mode)
{
	pr_info("mode : %x \n", access_mode);
	if(dev_perm == RDWR){
		pr_info("File is opened with RDWR\n");
		return 0;
	}
	/* Ensures readonly permission */
	if( (dev_perm == RDONLY) && ( (access_mode & FMODE_READ) && !(access_mode & FMODE_WRITE) ) ){
		pr_info("File is opened with RDONLY\n");
		return 0;
	}
	/* Ensures writeonly permission */
	if( (dev_perm == WRONLY) && ( (access_mode & FMODE_WRITE) && !(access_mode & FMODE_READ) ) ){
		pr_info("File is opened with WRONLY\n");
                return 0;
        }

	pr_err("Permission denied \n");
	return -EPERM;
}



int pcd_open(struct inode *inode, struct file *filp){
  	int ret;
	
	int minor_n;
	
	struct pcdev_private_data *pcdev_data;
	
	/* Find out on which device file open was attempted by the user space */
	minor_n = MINOR(inode->i_rdev);
	pr_info("minor access = %d\n", minor_n);
	
	/* get device's private data structure */	
	pcdev_data = container_of(inode->i_cdev,struct pcdev_private_data, cdev);
	
	/* To supply device private data to other methods of the driver */
	filp->private_data = pcdev_data;
	
	/* Check permission */
	pr_info("pcdev perm = %d | file perm = %d \n", pcdev_data->perm, filp->f_mode);
	ret = check_permission(pcdev_data->perm, filp->f_mode);

	(!ret) ? pr_info("open was successful\n") : pr_info("open was unsuccessful\n");
  	return ret;
}

int pcd_release(struct inode *inode, struct file *filp){
        pr_info("release was successful \n");
	return 0;
}

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence){
	
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	
	int max_size = pcdev_data->size;

	pr_info("lseek requested \n");
	pr_info("current value of file position = %lld \n", filp->f_pos);

	loff_t temp;
	if(whence == SEEK_SET){
		if((offset > max_size) || (offset < 0))
			return -EINVAL;
		filp->f_pos = offset;
	}
	else if(whence == SEEK_CUR){
		temp = filp->f_pos + offset;
		if((temp > max_size) || (temp < 0))
			return -EINVAL;
		filp->f_pos = temp;
	}
	else if(whence == SEEK_END){
		temp = max_size + offset;
                if((temp > max_size) || (temp < 0))
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
	
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	pr_info("read requested for %zu bytes \n", count);
	pr_info("current value of file position = %lld \n", *f_pos);
	int max_size = pcdev_data->size;
	
	/* Adjust the 'count */
	if((count + *f_pos) > max_size){
		count = max_size - *f_pos;
	}

	/* Copy to user */
	if(copy_to_user(buff, &pcdev_data->buffer[*f_pos], count)){
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
	
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	pr_info("write requested for %zu bytes \n", count);
	pr_info("current file position = %lld \n", *f_pos);
	
	int max_size = pcdev_data->size;
        /* Adjust the 'count */
        if((count + *f_pos) > max_size){
                count = max_size - *f_pos;
        }
	/* Return no memory if count is equal to zero */
	if(!count){
		pr_err("No space left on the device \n");
		return -ENOMEM;
	}

        /* Copy from user */
        if(copy_from_user(&pcdev_data->buffer[*f_pos], buff, count)){
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


static int __init pcd_init(void){

	int ret;
	/* Dynamically allocate a device number */
	
	ret = alloc_chrdev_region(&pcdrv_data.device_number,0,NO_DEVICES,"pcd_devices");
	
	if(ret < 0){
		pr_err("Alloc chrdev region failed\n");
		goto exit;
	}
	/* create device class under sys/class/ */
        pcdrv_data.class_pcd = class_create("pcd_class");
        if(IS_ERR(pcdrv_data.class_pcd)){
                pr_err("Class creation failed\n");
                ret = PTR_ERR(pcdrv_data.class_pcd);// converts Pointer to Error code, ptr -> int
                goto unreg_chrdev;
        }
	int i = 0;
	/* Initialize the cdev structures with fops */
	for(i = 0; i < NO_DEVICES ; i++){
		dev_t devno = MKDEV(MAJOR(pcdrv_data.device_number), MINOR(pcdrv_data.device_number) + i);
		pr_info("Device number <major>:<minor> = %d:%d\n",  MAJOR(devno), MINOR(devno));
		cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);
		pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;	// We've put it after the func because inside the 
                                      					// func all pcd_cdev struct is loaded 0
	
		/* Register a device (cdev structure) with VFS */
	
		ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, devno, 1);
		if(ret < 0){
			pr_err("Cdev add failed\n");
			goto class_del;
		}
	
		/* populate the sysfs with device information */
		pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, devno, NULL, "pcd_dev-%d", i+1);

		if(IS_ERR(pcdrv_data.device_pcd)){
			pr_err("Device creation failed\n");
			ret = PTR_ERR(pcdrv_data.device_pcd);
			cdev_del(&pcdrv_data.pcdev_data[i].cdev);
			goto class_del;
		}
	}
	pr_info("Module init was successful\n");
	return 0;
cdev_del :
class_del :
	while(--i >= 0){
		dev_t devno = MKDEV(MAJOR(pcdrv_data.device_number), MINOR(pcdrv_data.device_number) + i);
		device_destroy(pcdrv_data.class_pcd, devno);
		cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.class_pcd);
unreg_chrdev :
	unregister_chrdev_region(pcdrv_data.device_number, NO_DEVICES);	
exit :
	pr_err("Module insertion failed \n");
	return ret;
}

static void __exit pcd_exit(void){

	int i ;
	for(i = 0; i < NO_DEVICES ; i++){
		dev_t devno = MKDEV(MAJOR(pcdrv_data.device_number), MINOR(pcdrv_data.device_number) + i);
		/* Remove a device that was created with device_create() */
                device_destroy(pcdrv_data.class_pcd, devno);
		
		/* Remove cdev registration from the kernel VFS */
                cdev_del(&pcdrv_data.pcdev_data[i].cdev);
        }

	class_destroy(pcdrv_data.class_pcd);

	/* Unregister a range of device numbers */
	unregister_chrdev_region(pcdrv_data.device_number, NO_DEVICES);	

	pr_info("Module unload was successful\n");
}

module_init(pcd_init);
module_exit(pcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Youssef ELBATTAH");
MODULE_DESCRIPTION("Pseudo multi character device driver");
MODULE_INFO(board, "BBB with linux rev_5.168.1");
