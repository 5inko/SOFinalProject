#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h> 
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  //copy_to/from_user()
#include <linux/gpio.h>     //GPIO
#include <linux/err.h>
#include <linux/delay.h>

bool gpio_is_valid(int buzzer);

//Defining BUZZER tones and pin
#define GPIO_21 (21)

#define DO 200
#define RE 250
#define MI 300
#define FA 350
#define SOL 400
#define LA 450
#define SI 500

/*************** SONG **********************/
int imperial_march_melody[] = { 
  SOL, SOL, SOL, RE, LA, SOL, RE, LA, SOL,
  MI, MI, MI, RE, LA, SOL, RE, LA, SOL,
  SI, SI, SI, RE, LA, SOL, RE, LA, SOL,
  SOL, SOL, SOL, RE, LA, SOL, RE, LA, SOL,
  MI, MI, MI, RE, LA, SOL, RE, LA, SOL,
  SI, SI, SI, RE, LA, SOL, RE, LA, SOL,
  SOL, SOL, SOL, RE, LA, SOL, RE, LA, SOL,
  MI, MI, MI, RE, LA, SOL, RE, LA, SOL
};

int imperial_march_beat[] = {
  2, 1, 1, 1, 2, 1, 1, 2, 1,
  2, 1, 1, 1, 2, 1, 1, 2, 1,
  2, 1, 1, 1, 2, 1, 1, 2, 1,
  2, 1, 1, 1, 2, 1, 1, 2, 1,
  2, 1, 1, 1, 2, 1, 1, 2, 1,
  2, 1, 1, 1, 2, 1, 1, 2, 1,
  2, 1, 1, 1, 2, 1, 1, 2, 1,
  2, 1, 1, 1, 2, 1, 1, 2, 1
};
/******************************************************/
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
 
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
 
 
/*************** Driver functions **********************/
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, 
                char __user *buf, size_t len,loff_t * off);
static ssize_t etx_write(struct file *filp, 
                const char *buf, size_t len, loff_t * off);
/******************************************************/
 
//File operation structure 
static struct file_operations fops =
{
  .owner          = THIS_MODULE,
  .read           = etx_read,
  .write          = etx_write,
  .open           = etx_open,
  .release        = etx_release,
};

/*
** This function will be called when we open the Device file
*/ 
static int etx_open(struct inode *inode, struct file *file)
{
  pr_info("Device File Opened...!!!\n");
  return 0;
}

/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file)
{
  pr_info("Device File Closed...!!!\n");
  return 0;
}

/*
** This function will be called when we read the Device file
*/ 
static ssize_t etx_read(struct file *filp, 
                char __user *buf, size_t len, loff_t *off)
{
  // No necesitamos leer el estado del GPIO para este caso
  return 0;
}

/*
** This function will be called when we write the Device file
*/ 
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    uint8_t rec_buf[10] = {0};
    int i,j,duration;
  
    if (copy_from_user(rec_buf, buf, len) > 0) {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
        return -EFAULT;
    }

if (rec_buf[0] == '1') {
    pr_err("en el if\n");
        for (i = 0; i < sizeof(imperial_march_melody) / sizeof(imperial_march_melody[0]); i++) {
          pr_err("en el for1\n");
            duration = 1000 / (imperial_march_beat[i]); // calculate note duration
            for (j = 0; j < imperial_march_melody[i]; j++) {
                pr_err("en el for2\n");
                gpio_set_value(GPIO_21, 1);
                udelay(duration);  // delay for half the period of the tone
                gpio_set_value(GPIO_21, 0);
                udelay(duration);  // delay for half the period of the tone
            }
            mdelay(50);
    }
    } else {
        pr_err("Unknown command: Please provide '1' to play the Imperial March.\n");
        return -EINVAL;
    }

    return len;
}

/*
** Module Init function
*/ 
static int __init etx_driver_init(void)
{
  /*Allocating Major number*/
  if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
    pr_err("Cannot allocate major number\n");
    return -1;
  }
  pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
  /*Creating cdev structure*/
  cdev_init(&etx_cdev,&fops);
 
  /*Adding character device to the system*/
  if((cdev_add(&etx_cdev,dev,1)) < 0){
    pr_err("Cannot add the device to the system\n");
    goto r_del;
  }
 
  /*Creating struct class*/
  if(IS_ERR(dev_class = class_create(THIS_MODULE,"etx_class"))){
    pr_err("Cannot create the struct class\n");
    goto r_class;
  }
 
  /*Creating device*/
  if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"etx_device"))){
    pr_err( "Cannot create the Device \n");
    goto r_device;
  }
  
  //Checking the GPIO is valid or not
  if(gpio_is_valid(GPIO_21) == false){
    pr_err("GPIO %d is not valid\n", GPIO_21);
    goto r_device;
  }
  
  //Requesting the GPIO
  if(gpio_request(GPIO_21,"GPIO_21") < 0){
    pr_err("ERROR: GPIO %d request\n", GPIO_21);
    goto r_gpio;
  }
  
  //configure the GPIO as output
  gpio_direction_output(GPIO_21, 0);
  
  /* Using this call the GPIO 21 will be visible in /sys/class/gpio/
  ** Now you can change the gpio values by using below commands also.
  ** 
  ** the second argument prevents the direction from being changed.
  */
  gpio_export(GPIO_21, false);
  
  pr_info("Device Driver Insert...Done!!!\n");
  return 0;
 
r_gpio:
  gpio_free(GPIO_21);
r_device:
  device_destroy(dev_class,dev);
r_class:
  class_destroy(dev_class);
r_del:
  unregister_chrdev_region(dev,1);
  
  return -1;
}

/*
** Module exit function
*/ 
static void __exit etx_driver_exit(void)
{
  gpio_unexport(GPIO_21);
  gpio_free(GPIO_21);
  device_destroy(dev_class,dev);
  class_destroy(dev_class);
  cdev_del(&etx_cdev);
  unregister_chrdev_region(dev, 1);
  pr_info("Device Driver Remove...Done!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("GRUPO_OPERATIVOS");
MODULE_DESCRIPTION("A simple device driver - GPIO Driver");
MODULE_VERSION("1.32");

