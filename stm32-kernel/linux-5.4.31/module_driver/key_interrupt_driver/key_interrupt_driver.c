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

dev_t button_parent = 120;
static struct class *button_class = NULL;

enum button_status{
    KEY_PRESS = 0,
    KEY_RELEASE = 1,
    KEY_KEEP = 2,
};

typedef struct button_dev_s{
    dev_t devid;            //设备号
    struct cdev cdev;       //字符设备
    struct device *device;  //设备
    struct device_node *nd; //设备节点
    int gpio_id;            //gpio号
    int irq_num;            //中断号
    spinlock_t spinlock;    //自旋锁
    enum button_status status;  //状态
    const char* label;            //设备名字
    
} button_dev;

button_dev button[2];
struct timer_list button_timer;//定时器
enum button_status last_status = KEY_KEEP;

/* irq handle */
static irqreturn_t button_irq_handler(int irq, void *dev_id){
    int i = 0;
    for (i=0; i<BUTTONDEV_CNT; i++){
        if (button[i].irq_num == irq){
            button_timer.flags = i;
            break;
        }
    }
    mod_timer(&button_timer, jiffies + msecs_to_jiffies(15));
    return 0;
}

static void key_timer_function(struct timer_list *arg){
    int i = 0;
    button_dev *dev = NULL;
    unsigned long flags = 0;
    int current_val = 0;

    for (i=0; i<BUTTONDEV_CNT; i++){
        if (i == arg->flags){
            dev = &button[i];
            break;
        }
    }

    spin_lock_irqsave(&dev->spinlock, flags);
    
    current_val = gpio_get_value(dev->gpio_id);
    if (current_val == 0 && last_status == 1){
        dev->status = KEY_PRESS;
    }
    else if (current_val == 1 && last_status == 0){
        dev->status = KEY_RELEASE;
    }
    else{
        dev->status = KEY_KEEP;
    }
    last_status = current_val;

    spin_unlock_irqrestore(&dev->spinlock, flags);
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
    button_dev *dev = NULL;

    if ( filp == NULL ){
        pr_err("button:get no file\r\n");
        return -EINVAL;
    }

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

static struct file_operations button_fops = {
    .owner = THIS_MODULE,
    .open = button_open,
    .read = button_read,
    .release = button_release,
};

static int button_parse_dt(void){
    struct device_node *parent_node = NULL;
    struct device_node *ptr = NULL;
    int i = 0;
    int ret = 0;
    
    parent_node = of_find_node_by_name(NULL, BUTTONDEV_NAME);
    if (parent_node == NULL){
        pr_err("button:parse device tree failed\n");
        return -EINVAL;
    }
    
    for (i=0; i<BUTTONDEV_CNT; i++){
        button[i].nd = of_get_next_child(parent_node, ptr);
        ptr = button[i].nd;

        ret = of_property_read_string(ptr, "label", &button[i].label);
        if (ret < 0){
            pr_err("button:failed to get label\n");
            return -EINVAL;
        }
        button[i].gpio_id = of_get_gpio(ptr, 0);
        if (!gpio_is_valid(button[i].gpio_id)){
            pr_err("button:failed to get gpio\n");
            return -EINVAL;
        }
        button[i].irq_num = irq_of_parse_and_map(ptr, 0);
        if (!button[i].irq_num){
            pr_err("button:failed to get irq_num\n");
            return -EINVAL;
        }
    }
    return 0;
}

static int button_exit_init(button_dev *dev){
    int ret = 0;
    u32 irq_flags = 0;

    ret = gpio_request(dev->gpio_id, dev->label);
    if (ret < 0){
        pr_err("button:failed to request gpio\n");
        return ret;
    }

    gpio_direction_input(dev->gpio_id);

    irq_flags = irq_get_trigger_type(dev->irq_num);
    if (irq_flags == IRQF_TRIGGER_NONE){
        irq_flags = IRQF_TRIGGER_FALLING;
    }

    ret = request_irq(dev->irq_num, button_irq_handler, irq_flags, dev->label, dev);
    if (ret){
        gpio_free(dev->gpio_id);
        return ret;
    }

    return 0;
}

static int button_probe(struct platform_device *pdev){
    int ret = 0;
    int i = 0;
    button_parse_dt();

    //初始化button gpio、中断
    for (i=0; i<BUTTONDEV_CNT; i++){
        button_exit_init(&button[i]);
    }

    //申请设备号
    ret = alloc_chrdev_region(&button_parent, 0, BUTTONDEV_CNT, BUTTONDEV_NAME);
    if (ret < 0){
        pr_err("button:alloc chrdev failed\n");
        goto free_gpio;
    }

    //创建字符设备注册到内核
    for (i=0; i<BUTTONDEV_CNT; i++){
        button[i].devid = MKDEV(MAJOR(button_parent), i);

        cdev_init(&button[i].cdev, &button_fops);

        ret = cdev_add(&button[i].cdev, button[i].devid, 1);
        if (ret < 0){
            pr_err("button:register into kernel failed\n");
            goto unregister_devid;
        }
    }

    //创建类注册到内核
    button_class = class_create(THIS_MODULE, BUTTONDEV_NAME);
    if (ret < 0){
        pr_err("button:create class failed\n");
        goto del_cdev;
    }

    //创建设备
    for (i=0; i<BUTTONDEV_CNT; i++){
        button[i].device = device_create(button_class, NULL, button[i].devid, NULL, button[i].label);
        if (ret < 0){
            pr_err("button:create device failed\r\n");
            goto del_class;
        }
    }
    for (i=0; i<BUTTONDEV_CNT; i++){
        timer_setup(&button_timer, key_timer_function, 0);
    }
    
    return 0;

del_class:
    class_destroy(button_class);
del_cdev:
    for (i=0; i<BUTTONDEV_CNT; i++){
        cdev_del(&button[i].cdev);
    }
unregister_devid:
    unregister_chrdev_region(button_parent, BUTTONDEV_CNT);
free_gpio:
    for (i=0; i<BUTTONDEV_CNT; i++){
        gpio_free(button[i].gpio_id);
    }
    return -EIO;
}

static int button_remove(struct platform_device *pdev){
    int i = 0;

    device_destroy(button_class, button_parent);
    class_destroy(button_class);
    for (i=0; i<BUTTONDEV_CNT; i++){
        cdev_del(&button[i].cdev);
    }
    unregister_chrdev_region(button_parent, BUTTONDEV_CNT);
    for (i=0; i<BUTTONDEV_CNT; i++){
        gpio_free(button[i].gpio_id);
    }
    return 0;
}


static struct of_device_id button_of_match[] = {
    { .compatible = "learn,button" },
    {}
};

MODULE_DEVICE_TABLE(of, button_of_match);

static struct platform_driver button_driver = {
    .driver = {
        .name = "button-learn-driver",
        .of_match_table = button_of_match,
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