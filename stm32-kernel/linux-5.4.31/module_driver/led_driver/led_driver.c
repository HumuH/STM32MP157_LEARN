#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>



#define LED_MAJOR 256
#define LED_MINOR 0
#define LEDDEV_CNT  2
#define LEDDEV_NAME "led_learn"
#define LEDDEV_GROUP_PATH "gpioleds-learn"

#define LEDOFF      1
#define LEDON       0

dev_t parent_devid = 0;
struct class *led_class = NULL;

typedef struct leddev_dev_s{
    dev_t devid;                //设备号
    struct cdev cdev;           //字符设备
    struct class *class;        //类
    struct device *device;      //设备
    struct device_node *node;   //设备节点
    int gpio_id;                //gpio号
    const char *label;                  //设备名字
} led_dev;

led_dev led[LEDDEV_CNT];

static int led_open(struct inode *inode, struct file *filp){
    if (inode == NULL){
        return -1;
    }
    if (filp == NULL){
        return -1;
    }
    filp->private_data = container_of(inode->i_cdev, led_dev, cdev);
    
    return 0;
}

static int led_release(struct inode *inode, struct file *filp){
    filp->private_data = NULL;
    return 0;
}


static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt){
    
    int ret = 0;
    u8 databuf[1];
    u8 led_status = 0;
    led_dev* dev = (led_dev*)filp->private_data;

    ret = copy_from_user(databuf, buf, cnt);
    if (ret < 0){
        pr_err("kernel write failed!\r\n");
        return -EFAULT;
    }
    led_status = databuf[0];
    if (led_status == LEDON || led_status == LEDOFF){
        gpio_set_value(dev->gpio_id, led_status);
    }

    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt){
    char led_status = 0;
    led_dev* dev = NULL;
    dev = (led_dev*)filp->private_data;

    if (dev == NULL){
        pr_err("kernel write failed!\r\n");
        return -EFAULT;
    }
    

    led_status = gpio_get_value(dev->gpio_id);
    buf[0] = led_status;

    return 1;
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
    .read = led_read,
    .release = led_release,
};

static int led_gpio_init(led_dev *dev){
    int ret = 0;
    
    const char *default_status;
    
    dev->gpio_id = of_get_named_gpio(dev->node, "gpio", 0);

    if (!gpio_is_valid(dev->gpio_id)){
        pr_err("led:failed to get led gpio\r\n");
        return -EINVAL;
    }

    ret = of_property_read_string(dev->node, "label", &(dev->label));
    if (ret < 0){
        pr_err("led:failed to get dev label\r\n");
        return -EINVAL;
    }
    
    ret = gpio_request(dev->gpio_id, dev->label);
    if (ret < 0){
        pr_err("led:failed to request led gpio\r\n");
        return ret;
    }
    
    ret = of_property_read_string(dev->node, "default-status", &default_status);
    if (ret < 0){
        pr_err("led:failed to get dev default-status\r\n");
        return -EINVAL;
    }

    if (strcmp(default_status, "on") == 0){
        gpio_direction_output(dev->gpio_id, LEDON);
    }
    else{
        gpio_direction_output(dev->gpio_id, LEDOFF);
    }
    
    return 0;
}

static int led_probe(struct platform_device *pdev){
    int ret = 0;
    int i = 0;

    struct device_node *parent_node = NULL;
    struct device_node *ptr = NULL;
    int major_devid = 0;
    
    parent_node = of_find_node_by_name(NULL, LEDDEV_GROUP_PATH);
    if (parent_node == NULL){
        pr_err("led: can't find led gpio group\r\n");
        return ret;
    }
    
    for (i=0; i<LEDDEV_CNT; i++){
        led[i].node = of_get_next_child(parent_node, ptr);
        ptr = led[i].node;
        ret = led_gpio_init(&(led[i]));
        if (ret < 0){
            pr_err("led: failed to init\r\n");
            return ret;
        }
    }
    
    ret = alloc_chrdev_region(&parent_devid, 0, LEDDEV_CNT, LEDDEV_NAME);
    if (ret < 0){
        pr_err("led: failed to alloc chrdev region\r\n");
        goto free_gpio;
    }
    
    major_devid = MAJOR(parent_devid);
    
    /* 创建字符设备，注册到内核中 */
    for (i=0; i<LEDDEV_CNT; i++){
        led[i].devid = parent_devid + i;

        led[i].cdev.owner = THIS_MODULE;
        
        cdev_init(&led[i].cdev, &led_fops);

        ret = cdev_add(&led[i].cdev, led[i].devid, 1);
        if (ret < 0){
            pr_err("led:cdev add failed\r\n");
            goto cdev_unregister;
        }
    }

    /* 创建设备的类，注册到内核中 */
    led_class = class_create(THIS_MODULE, LEDDEV_NAME);

    if (IS_ERR(led_class)){
        pr_err("led:class create failed\r\n");
        goto cdev_del;
    }

    /* 创建设备 */
    for (i=0; i<LEDDEV_CNT; i++){
        led[i].device = device_create(led_class, NULL, led[i].devid, NULL, led[i].label);
        if (IS_ERR(led[i].device)){
            pr_err("led:device create failed\r\n");
            goto class_del;
        }
    }
    pr_err("led driver and device was matched!\r\n");
    return 0;

class_del:
    class_destroy(led_class);
cdev_del:
    for (i=0; i<LEDDEV_CNT; i++){
        cdev_del(&led[i].cdev);
    }
    
cdev_unregister:
    unregister_chrdev_region(parent_devid, LEDDEV_CNT);
free_gpio:
    for (i=0; i<LEDDEV_CNT; i++){
        gpio_free(led[i].gpio_id);
    }

    return -EIO;
}

static int led_remove(struct platform_device *dev){
    int i = 0;
    
    for (i=0; i<LEDDEV_CNT; i++){
        gpio_direction_output(led[i].gpio_id, LEDOFF);
        gpio_free(led[i].gpio_id);
        device_destroy(led_class, led[i].devid);
    }
    
    class_destroy(led_class);

    for (i=0; i<LEDDEV_CNT; i++){
        cdev_del(&led[i].cdev);
    }

    unregister_chrdev_region(parent_devid, LEDDEV_CNT);

    return 0;
}


static const struct of_device_id led_of_match[] = {
    { .compatible = "learn,led" },
    {  }
};

MODULE_DEVICE_TABLE(of, led_of_match);

static struct platform_driver led_driver = {
    .driver = {
        .name = "led-learn-driver",
        .of_match_table = led_of_match,
    },
    .probe = led_probe,
    .remove = led_remove,
};

static int __init leddriver_init(void){
    return platform_driver_register(&led_driver);
}

static void __exit leddriver_exit(void){
    platform_driver_unregister(&led_driver);
}


module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SQQ");
MODULE_INFO(intree, "Y");
