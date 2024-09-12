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


#define LEDDEV_CNT  2
#define LEDDEV_NAME "gpioled_devname"
#define LEDOFF      0
#define LEDON       1

typedef struct leddev_dev_s{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    int gpio_led;
} led_dev;

led_dev led;

static int led_open(struct inode *inode, struct file *filp){
    if (filp == NULL){
        return -1;
    }
    filp->private_data = &led;
    pr_err("led device open!\r\n");
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
    led_dev* led = (led_dev*)filp->private_data;

    ret = copy_from_user(databuf, buf, cnt);
    if (ret < 0){
        printk(KERN_ERR, "kernel write failed!\r\n");
        return -EFAULT;
    }
    led_status = databuf[0];
    if (led_status == LEDON || led_status == LEDOFF){
        gpio_set_value(led->gpio_led, led_status);
    }

    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt){
    led_dev* led = (led_dev*)filp->private_data;
    if (led == NULL){
        printk(KERN_ERR, "kernel write failed!\r\n");
        return -EFAULT;
    }
    u8 led_status = 0;

    led_status = gpio_get_value(led->gpio_led);
    buf[0] = led_status;

    return 0;
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
    .read = led_read,
    .release = led_release,
};

static int led_gpio_init(struct device_node *nd){
    int ret = 0;
    led.gpio_led = of_get_named_gpio(nd, "gpios", 0);
    if (!gpio_is_valid(led.gpio_led)){
        pr_err("led: failed to get gpioled\r\n");
        return -EINVAL;
    }
    ret = gpio_request(led.gpio_led, "LED0");
    if (ret){
        pr_err("led: failed to request gpioled\r\n");
        return ret;
    }
    gpio_direction_output(led.gpio_led, 0);
    return 0;
}

static int led_probe(struct platform_device *pdev){
    int ret = 0;

    ret = led_gpio_init(pdev->dev.of_node);
    if (ret < 0){
        pr_err("led: failed to init\r\n");
        return ret;
    }

    ret = alloc_chrdev_region(&led.devid, 0, LEDDEV_CNT, LEDDEV_NAME);
    if (ret < 0){
        pr_err("led: failed to alloc chrdev region\r\n");
        goto free_gpio;
    }
    
    led.cdev.owner = THIS_MODULE;
    cdev_init(&led.cdev, &led_fops);

    ret = cdev_add(&led.cdev, led.devid, LEDDEV_CNT);

    if (ret < 0){
        pr_err("led:cdev add failed\r\n");
        goto cdev_unregister;
    }

    led.class = class_create(THIS_MODULE, LEDDEV_NAME);
    if (IS_ERR(led.class)){
        pr_err("led:class create failed\r\n");
        goto cdev_del;
    }

    led.device = device_create(led.class, NULL, led.devid, NULL, LEDDEV_NAME);
    if (IS_ERR(led.device)){
        pr_err("led:device create failed\r\n");
        goto class_del;
    }

    pr_err("led driver and device was matched!\r\n");
    return 0;

class_del:
    class_destroy(led.class);
cdev_del:
    cdev_del(&led.cdev);
cdev_unregister:
    unregister_chrdev_region(led.devid, LEDDEV_CNT);
free_gpio:
    gpio_free(led.gpio_led);
    return -EIO;
}

static int led_remove(struct platform_device *dev){
    device_destroy(led.class, led.devid);
    class_destroy(led.class);
    cdev_del(&led.cdev);
    unregister_chrdev_region(led.devid, LEDDEV_CNT);
    gpio_free(led.gpio_led);
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
    pr_err("led:start probe\r\n");
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
