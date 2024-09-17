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
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/semaphore.h>

#define BUTTONDEV_CNT 2
#define BUTTONDEV_NAME "button_learn"

dev_t button_major = 1;


enum button_status{
    KEY_PRESS = 0,
    KEY_KEEP = 1,
    KEY_RELEASE = 2,
};

typedef struct button_dev_s{
    dev_t devid;            //设备号
    struct cdev cdev;       //字符设备
    struct class *class;    //类
    struct device *device;  //设备
    struct device_node *nd; //设备节点
    int gpio_id;            //gpio号
    struct timer_list timer;//定时器
    int irq_num;            //中断号
    spinlock_t spinlock;    //自旋锁
    enum button_status status;  //状态
    char *label;            //设备名字
} button_dev;

button_dev button[2];

/* irq handle */
button_irq_handler(int num, void *dev){

}

static int button_open(struct inode *inode, struct file *filp){
    if ( inode == NULL || filp == NULL ){
        pr_err("button:get no file\r\n");
        return -EINVAL;
    }
    filp->private_data = container_of(inode->i_cdev, button_dev, cdev);
    return 0;
}

static ssize_t button_read(struct file *filp, char __user *buf, size_t cnt, loff_t *loft){
    if ( filp == NULL ){
        pr_err("button:get no file\r\n");
        return -EINVAL;
    }
    button_dev *dev = NULL;
    dev = (button_dev*)filp->private_data;

    buf[0] = dev->status;

    return 1;
}

static int button_release(struct inode *inode, struct file *filp){
    if ( inode == NULL || filp == NULL ){
        pr_err("button:get no file\r\n");
        return -EINVAL;
    }
    filp->private_data = NULL;
    return 0;
}

static struct file_operations = {
    .owner = THIS_MODULE,
    .open = button_open,
    .read = button_read,
    .release = button_release,
};

static int button_parse_dt(void){
    struct device *parent_node = NULL;
    struct device *ptr = NULL;
    int i = 0;
    int ret = 0;
    
    parent_node = of_find_node_by_name(NULL, BUTTONDEV_NAME);
    
    for (i=0; i<BUTTONDEV_CNT; i++){
        button[i].nd = of_get_next_child(parent_node, ptr);
        ptr = button[i].nd;

        ret = of_property_read_string(ptr, "label", &(button[i].label));
        button[i].gpio_id = of_get_gpio(ptr, 0);
        button[i].irq_num = irq_of_parse_and_map(ptr, 0);
    }
    return 0;
}

static int button_exit_init(button_dev *dev){
    int ret = 0;
    u32 irq_flags = 0;

    ret = gpio_request(dev->gpio_id, dev->label);

    gpio_direction_input(dev->gpio_id);

    irq_flags = irq_get_trigger_type(dev->irq_num);

    ret = request_irq(dev->irq_num, button_irq_handler, irq_flags, dev->label, dev);

    return 0;
}

static int button_probe(struct platform_device *pdev){
    int ret = 0;
    int i = 0;
    button_parse_dt();

    //初始化button gpio、中断
    for (i=0; i<BUTTONDEV_CNT; i++){
        button_exit_init(button[i].nd);
    }

    ret = alloc_chrdev_region(&button_major, 0, BUTTONDEV_CNT, BUTTONDEV_NAME);

    

}

static int button_remove(struct platform_device *pdev){

}


static struct of_device_id button_of_match[] = {
    { .compatible = "learn,button" },
    {}
};

MODULE_DEVICE_TABLE(of, button_of_match);

static struct platform_driver button_driver = {
    .driver = {
        .name = "button-learn-driver",
        .of_match_table = &of_match_button,
    },
    .probe = button_probe,
    .remove = button_remove,
};

static int __init button_driver_init(void){
    return platform_driver_register(&button_driver);
}

static void __exit button_driver_exit(void){
    platform_driver_unregister(&button_driver);
}

module_init(button_driver_init);
module_exit(button_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SQQ");
MODULE_INFO(intree, "Y");