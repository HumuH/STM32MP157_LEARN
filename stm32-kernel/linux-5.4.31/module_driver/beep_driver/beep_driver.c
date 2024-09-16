#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/kernel.h>
#include <linux/ide.h>

#define BEEPDEV_CNT 1
#define BEEPDEV_NAME "beep"
#define BEEPDEV_MINOR   144

#define BEEP_ON 0
#define BEEP_OFF 1

typedef struct beep_dev_s{
    int gpio_id;
} beep_dev;

beep_dev beep;

static int beep_open(struct inode *inode, struct file *filp){
    filp->private_data = &beep;
    return 0;
}

static ssize_t beep_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt){
    beep_dev *dev = NULL;
    char beep_status = 0;
    
    dev = (beep_dev*)filp->private_data;

    beep_status = gpio_get_value(dev->gpio_id);

    buf[0] = beep_status;

    return 1;

}
static ssize_t beep_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt){
    char buffer[1] = {0};
    int ret = 0;
    char beep_status = 0;
    beep_dev *dev = (beep_dev*)filp->private_data;

    ret = copy_from_user(buffer, buf, cnt);
    if (ret < 0){
        pr_err("beep:write no data\r\n");
        return -EINVAL;
    }
    beep_status = buffer[0];

    if (beep_status == BEEP_OFF || beep_status == BEEP_ON){
        gpio_direction_output(dev->gpio_id, beep_status);
    }
    
    return 0;
}

static struct file_operations beep_fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .write = beep_write,
    .read = beep_read,
};

static struct miscdevice beep_miscdev = {
    .minor = BEEPDEV_MINOR,
    .name = BEEPDEV_NAME,
    .fops = &beep_fops,
};

static int beep_gpio_init(struct device_node *dev_node){
    int ret = 0;
    
    beep.gpio_id = of_get_named_gpio(dev_node, "gpio", 0);
    if (!gpio_is_valid(beep.gpio_id)){
        pr_err("beep:not find property of gpio");
        return -ENAVAIL;
    }

    ret = gpio_request(beep.gpio_id, BEEPDEV_NAME);
    if (ret < 0){
        pr_err("beep:failed to request gpio");
        return ret;
    }

    gpio_direction_output(beep.gpio_id, BEEP_OFF);
    return 0;
}

static int beep_probe(struct platform_device *pdev){
    int ret = 0;
    ret = beep_gpio_init(pdev->dev.of_node);
    if (ret < 0){
        pr_err("beep: gpio init failed\r\n");
        return ret;
    }

    ret = misc_register(&beep_miscdev);
    if (ret < 0){
        pr_err("beep: misc_register failed\r\n");
        goto free_gpio;
    }
    pr_err("beep driver and device was matched!\r\n");
    return 0;

free_gpio:
    gpio_free(beep.gpio_id);
    return -EINVAL;
}

static int beep_remove(struct platform_device *pdev){
    gpio_direction_output(beep.gpio_id, BEEP_OFF);
    gpio_free(beep.gpio_id);
    misc_deregister(&beep_miscdev);
    return 0;
}

static const struct of_device_id beep_of_match[] = {
    { .compatible = "learn,beep" },
    {  }
};

MODULE_DEVICE_TABLE(of, beep_of_match);

static struct platform_driver beep_driver = {
    .driver = {
        .name = "beep-learn-driver",
        .of_match_table = beep_of_match,
    },
    .probe = beep_probe,
    .remove = beep_remove,
};

static int __init beep_driver_init(void){
    return platform_driver_register(&beep_driver);
}

static void __exit beep_driver_exit(void){
    platform_driver_unregister(&beep_driver);
}

module_init(beep_driver_init);
module_exit(beep_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SQQ");
MODULE_INFO(intree, "Y");