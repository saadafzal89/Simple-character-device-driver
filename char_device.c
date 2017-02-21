#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include<linux/semaphore.h>
#include <linux/uaccess.h>
#include"header_assign4.h"

#define DEVICE_NAME "mycdrv" //naming the device 
#define ramdisk_size (size_t) 100
#define MAGIC_NUM 'Z'
#define CHGACCDIR  1

static LIST_HEAD(list_driver);
static struct class *test_class = NULL;

dev_t device_num, devnum;
int ret_val, major_num, minor_num;

int count = 3;
module_param(count, int, 0);

int current_dir =0;	//initializing the initial direction as forward

struct asp_mycdrv
{
    struct list_head list;
    struct cdev dev;
    char *ramdisk;
    struct semaphore sem;
    int devNo;
};


/****************************************************************************************/
//Function for opening the device
int device_open(struct inode *inode, struct file *file_p)
{
    struct asp_mycdrv *cdrv = container_of(inode->i_cdev, struct asp_mycdrv, dev);
	printk(KERN_INFO "newDevice: Device is opened");
	file_p->private_data = cdrv; 
	return 0;
}



/****************************************************************************************/
/*//Function for releasing the device
static int device_release(struct inode *inode, struct file *file_p) 
{
	struct asp_mycdrv *cdrv = file_p->private_data;
	pr_info(" Releasing device: mycdrv%d\n\n", cdrv->devNo);
	return 0;
}
*/

/****************************************************************************************/
//Function for reading the device
ssize_t device_read(struct file *file_p, char *buf, size_t buffer_count, loff_t *offset_val)
{
		
	int i=0;
	int num_of_bytes=0;

    	struct asp_mycdrv *cdrv = file_p->private_data;
    	if(down_interruptible(&cdrv->sem)!=0)
	{
		printk(KERN_ALERT "Error: the device could not be locked while opening\n");
		return -1;
	}


	pr_info("Inside read function, Buffer_Count = %d, Current Offset = %d\n", (int)buffer_count, (int)*offset_val);
	
	//checking for out of bounds condition in forward direction while reading
	if ((buffer_count + *offset_val) > ramdisk_size && current_dir == 0) 
    	{
		pr_info("Error -> trying to read beyond the end of file\n"); 
		up(&cdrv->sem);
		return 0;
	}
	
	//checking for out of bounds condition in reverse direction while reading
	if ( ((long)(*offset_val) - (long)buffer_count) < 0 && current_dir == 1)
    	{
		pr_info("Error -> trying to read beyond the end of file in reverse direction\n");
		up(&cdrv->sem);
		return 0;
    	}

	//reading in forward direction
    	if (current_dir == 0)
    	{
	    pr_info("Device is being read in forward direction\n");
	    num_of_bytes = buffer_count - copy_to_user(buf, cdrv->ramdisk + *offset_val, buffer_count);
	    *offset_val += num_of_bytes;
    	}
    
    	//reading in reverse direction
    	else
    	{
		pr_info("Device is being read in reverese direction\n");
        	for(i=0; i<buffer_count; i++)
        	{
	        	copy_to_user(buf+i, cdrv->ramdisk + *offset_val - i, 1);
	        	//pr_info("Returned value = %s\n", buf[i]);
            		num_of_bytes++;
        	}
	
	pr_info("Returned value = %s\n", buf);
        *offset_val -= num_of_bytes;        
    }
    
	//pr_info("In read function with  number of bytes = %d, Offset value = %d\n", num_of_bytes, (int)*offset_val);
    up(&cdrv->sem);
	return num_of_bytes;
}


/****************************************************************************************/
//Function for writing the device
ssize_t device_write(struct file *file_p, const char *buf, size_t buffer_count, loff_t *offset_val)
{
    	
	int i=0;
	int num_of_bytes=0;

	struct asp_mycdrv *cdrv = file_p->private_data;
    	if(down_interruptible(&cdrv->sem)!=0)
	{
		printk(KERN_ALERT "Error: the device could not be locked while opening\n");
		return -1;
	}
	
	

	pr_info("In write function with  Buffer Count = %d, Current Offset = %d\n", (int)buffer_count, (int)*offset_val);
    
        //checking for out of bounds condition in forward direction while writing
	if ((buffer_count + *offset_val) > ramdisk_size && current_dir == 0) 
        {  
		pr_info("Error -> trying to read beyond the end of file\n");
		up(&cdrv->sem);
		return 0;
	}
	
	//checking for out of bounds condition in reverse direction while writing
	if ( ((long)(*offset_val) - (long)buffer_count) < 0 && current_dir == 1)
        {
		pr_info("Error -> trying to read beyond the end of file in reverse direction\n");
		up(&cdrv->sem);
		return 0;
	}

   	//writing in forward direction
    	if (current_dir == 0)
   	{ 
	    num_of_bytes = buffer_count - copy_from_user(cdrv->ramdisk + *offset_val, buf, buffer_count);
	    *offset_val += num_of_bytes;
    	}
    
    	//writing on reverse direction
    	else
    	{
        	for(i=0; i<buffer_count; i++)
        	{
	        	copy_from_user(cdrv->ramdisk + *offset_val - i, buf + i, 1);
            		num_of_bytes++;
        	}
        	*offset_val -= num_of_bytes;
    	}
        
	//pr_info("\In writing function with number of bytes = %d, Offset value = %d\n", num_of_bytes, (int)*offset_val);
	up(&cdrv->sem);
	return num_of_bytes;
}


/****************************************************************************************/
//Function for a seeking the file pointer 
loff_t device_lseek(struct file *file_p, loff_t offset_val, int argu)
{
	off_t position;
   	struct asp_mycdrv *cdrv = file_p->private_data;
    	if(down_interruptible(&cdrv->sem)!=0)
	{
		printk(KERN_ALERT "Error: the device could not be locked while opening\n");
		return -1;
	}
	
	
	printk(KERN_ALERT "Inside lseek: seeking with arguement = %d",argu);
    
    
    	//setting the value for offset depending on user input
	switch (argu) {
	
	case 0:
		position = offset_val;
		printk(KERN_ALERT "The position for case 0 is: %d\n",(int)position);
		break;
		
	case 1:
		position = file_p->f_pos + offset_val;
	    printk(KERN_ALERT "The position for case 1 is: %d\n",(int)position);
		break;
		
	case 2:
		position = ramdisk_size + offset_val;
		printk(KERN_ALERT "The position for case 2 is: %d\n",(int)position);
		break;
		
	default:
		return -EINVAL;
	}
	
    	//setting pointer to start of buffer incase user input <0
    	if (position < 0)
    	position = 0;
    
   	 //setting pointer to end of buffer incase user input > size
   	 if (position > ramdisk_size)
        position = ramdisk_size;
	
    	file_p->f_pos = position;
	
   	pr_info("Seeking to position = %ld\n", (long)position);
	up(&cdrv->sem);
	return position;
}



/****************************************************************************************/
//Function for chanding the direction 
long device_ioctl(struct file *file_p, unsigned int cmd, unsigned long arg) 
{	
	int dir,old_dir;

    	struct asp_mycdrv *cdrv = file_p->private_data;
    	if(down_interruptible(&cdrv->sem)!=0)
	{
		printk(KERN_ALERT "Error: the device could not be locked while opening\n");
		return -1;
	}
   	
   
   	if (_IOC_TYPE(cmd) == MAGIC_NUM  && _IOC_NR(cmd) == CHGACCDIR) 
   	{
        	dir = (int)arg;
        	printk(KERN_INFO "Inside IOCTL, setting direction = %d",dir);
        
        	//Error returned if direction is not 0 or 1
        	if(dir != 0 && dir != 1)
        	{
            		printk(KERN_INFO "Error: Invalid option for direction = %d for newDevice = %d",dir,cdrv->devNo);
            		return -1;
        	}
        
        	//swapping the values
        	old_dir = current_dir;
        	current_dir = dir;
        	//printk(KERN_INFO "Exiting the ioctl\n");
        	up(&cdrv->sem);
        	return old_dir;
   	}	
   	return -1;
}


/****************************************************************************************/
//Function to close the device
int device_close(struct inode *inode, struct file *file_p)
{
	struct asp_mycdrv *cdrv = file_p->private_data;
	pr_info(" Closing device: mycdrv%d\n\n", cdrv->devNo);
	return 0;
}



struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_close,
	.write = device_write,
	.read = device_read,
	.llseek=device_lseek,
        .unlocked_ioctl = device_ioctl
};


/****************************************************************************************/
//Entry Function -> invoked on module_init
static int driver_entry(void)
{
    int i = 0;
    char device_name[20];
    struct asp_mycdrv *var;

    ret_val = alloc_chrdev_region(&devnum, 0, count, DEVICE_NAME);

    major_num = MAJOR(devnum);
    minor_num = MINOR(devnum);

    //Unregistering the device incase class is not created
    if ((test_class = class_create(THIS_MODULE, "chardrv")) == NULL)  
    {
        unregister_chrdev_region(device_num, 1);
        return -1;
    }

    for (i = 0; i < count; i++)
    {
        sprintf(device_name, "%s_%d", DEVICE_NAME, i);
        
        if (ret_val<0) {
	        printk(KERN_ALERT "Error for newDevice: Major number could not be allocated\n");
	        return ret_val;
        }

	//displaying all the major and minor numbers
        printk(KERN_INFO "newDevice: (Major_Num, Minor_Num)  = (%d, %d)\n", major_num, minor_num + i);
        
        //assigning major, minor numbers
	device_num = MKDEV(major_num, minor_num + i);

        var= (struct asp_mycdrv *)kmalloc(sizeof(struct asp_mycdrv), GFP_KERNEL);
        var->ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
        
        cdev_init(&var->dev, &fops);
        var->dev.ops = &fops;
        var->devNo = i;
    	var->dev.owner = THIS_MODULE;

        ret_val = cdev_add(&var->dev, device_num, 1);
    	if(ret_val<0)
        {
    		printk(KERN_ALERT "Error Device could not be added to the kernel\n");
        	return ret_val;
    	}
    	
        INIT_LIST_HEAD(&var->list);

        list_add(&(var->list), &(list_driver));
        sema_init(&var->sem,1);
        
        if (device_create(test_class, NULL, device_num, NULL, device_name) == NULL)
        {
            class_destroy(test_class);
            unregister_chrdev_region(device_num, 1);
            return -1;
        }
    }

    list_for_each_entry(var, &list_driver, list)
    {
		printk(KERN_ALERT "Major_Num = %d\n", MINOR(var->dev.dev));
    }

	return 0;
}


/****************************************************************************************/
//Exit function -> invoked on module_exit
static void driver_exit(void)
{
    struct list_head *pos, *q;
    struct asp_mycdrv *var;

	list_for_each_safe(pos, q, &list_driver)
	{
        var= list_entry(pos, struct asp_mycdrv, list);
        list_del(pos);
        cdev_del(&var->dev);
		device_num = var->dev.dev;                
        kfree(var->ramdisk);
		device_destroy(test_class, device_num);
        kfree(var);
        printk(KERN_ALERT " Device mycdrv_%d has been unloaded from kernel\n", MINOR(device_num));
	}
    unregister_chrdev_region(devnum, count);
    class_destroy(test_class);
}


module_init(driver_entry);
module_exit(driver_exit);

MODULE_LICENSE("GPL v2");
