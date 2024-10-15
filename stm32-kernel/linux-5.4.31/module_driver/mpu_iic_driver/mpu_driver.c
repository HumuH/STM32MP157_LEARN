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
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "mpu_driver.h"


#define MPU6050_NAME    "mpu6050"
#define MPU6050_CNT     1



typedef struct {
    struct i2c_client *client;
    dev_t dev_id;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int x_pos;
    int y_pos;
    int z_pos;
    int x_acc;
    int y_acc;
    int z_acc;
    u16 temp;
} mpu6050_dev;

static int mpu6050_write_regs(mpu6050_dev *dev, u8 reg, u8 val){
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client*)(dev->client);

    u8 tx_buf[2] = {0};
    tx_buf[0] = reg;
    tx_buf[1] = val;
    msg.addr = client->addr;
    msg.flags = 0;
    msg.len = 2;
    msg.buf = tx_buf;
    return i2c_transfer(client->adapter, &msg, 1);
}

static int mpu6050_read_regs(mpu6050_dev *dev, u8 reg, void *val, int len){
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client*)(dev->client);
    u8 tmp;
    int ret = 0;

    tmp = reg;
    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].buf = &tmp;
    msg[0].len = 1;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = val;
    msg[1].len = 1;

    ret = i2c_transfer(client->adapter, msg, 2);

    if (reg == 2){
        ret = 0;
    }
    else{
        printk("i2c error:rd failed,ret is %d, reg is %d, len is %d", ret, reg, len);
        ret = -EREMOTEIO;
    }
    return ret;
}

static int mpu6050_chip_init(mpu6050_dev *dev){
    mpu6050_write_regs(dev, MPU_REG_PWR_MGMT_1, MPU_MASK_PWR_MGMT_1_RESET);
    mpu6050_write_regs(dev, MPU_REG_PWR_MGMT_1, MPU_MASK_PWR_MGMT_1_WAKEUP);
    mpu6050_write_regs(dev, MPU_REG_CONFIG, 0x06);
    mpu6050_write_regs(dev, MPU_REG_SMPLRT_DIV, (MPU_MASK_SMPLRT_DIV_RATE & 0x7));
    mpu6050_write_regs(dev, MPU_REG_ACCEL_CONFIG, MPU_MASK_ACCEL_CONFIG_FS_SEL);
    mpu6050_write_regs(dev, MPU_REG_GYRO_CONFIG, MPU_MASK_GYRO_CONFIG_FS_SEL);
    return 0;
}

static int mpu6050_get_temp(mpu6050_dev *dev){
    u16 temp = 0;
    mpu6050_read_regs(dev, MPU_REG_TEMP_OUT_H, &temp, 1);
    dev->temp |= (temp << 8);
    mpu6050_read_regs(dev, MPU_REG_TEMP_OUT_L, &temp, 1);
    dev->temp |= temp;
    return 0;
}

static int mpu6050_get_gyro(mpu6050_dev *dev){
    u16 temp = 0;
    mpu6050_read_regs(dev, MPU_REG_GYRO_XOUT_H, &temp, 1);
    dev->x_pos |= (temp << 8);
    mpu6050_read_regs(dev, MPU_REG_GYRO_XOUT_L, &temp, 1);
    dev->x_pos |= temp;

    mpu6050_read_regs(dev, MPU_REG_GYRO_YOUT_H, &temp, 1);
    dev->y_pos |= (temp << 8);
    mpu6050_read_regs(dev, MPU_REG_GYRO_YOUT_L, &temp, 1);
    dev->y_pos |= temp;

    mpu6050_read_regs(dev, MPU_REG_GYRO_ZOUT_H, &temp, 1);
    dev->z_pos |= (temp << 8);
    mpu6050_read_regs(dev, MPU_REG_GYRO_ZOUT_L, &temp, 1);
    dev->z_pos |= temp;

    return 0;
}

static int mpu6050_get_accel(mpu6050_dev *dev){
    u16 temp = 0;
    
    mpu6050_read_regs(dev, MPU_REG_ACCEL_XOUT_H, &temp, 1);
    dev->x_acc |= (temp << 8);
    mpu6050_read_regs(dev, MPU_REG_ACCEL_XOUT_L, &temp, 1);
    dev->x_acc |= temp;

    mpu6050_read_regs(dev, MPU_REG_ACCEL_YOUT_H, &temp, 1);
    dev->y_acc |= (temp << 8);
    mpu6050_read_regs(dev, MPU_REG_ACCEL_YOUT_L, &temp, 1);
    dev->y_acc |= temp;

    mpu6050_read_regs(dev, MPU_REG_ACCEL_ZOUT_H, &temp, 1);
    dev->y_acc |= (temp << 8);
    mpu6050_read_regs(dev, MPU_REG_ACCEL_ZOUT_L, &temp, 1);
    dev->y_acc |= temp;

    return 0;
}

static int mpu6050_open(struct inode *nd, struct file *filp){
    struct cdev *cdev = filp->f_path.dentry->d_inode->i_cdev;
    mpu6050_dev *dev = container_of(cdev, mpu6050_dev, cdev);
    mpu6050_chip_init(dev);
    return 0;
}

static int mpu6050_release(struct inode *nd, struct file *filp){
    return 0;
}

static ssize_t mpu6050_read(struct file *filp, char __user *buf, size_t cnt, loff_t *loft){
    struct cdev *cdev = filp->f_path.dentry->d_inode->i_cdev;
    u8 str[256] = {0};
    mpu6050_dev *dev = container_of(cdev, mpu6050_dev, cdev);
    mpu6050_get_temp(dev);
    mpu6050_get_gyro(dev);
    mpu6050_get_accel(dev);
    
    sprintf(str, "mpu6050:temp--%d, gyro--x:%d y:%d z:%d, accel--x:%d y:%d z:%d\r\n", dev->temp, dev->x_pos, dev->y_pos, dev->z_pos, dev->x_acc, dev->y_acc, dev->z_acc);

    copy_to_user(buf, str, sizeof(str));

    return 0;
}


static const struct file_operations mpu6050_ops = {
    .owner = THIS_MODULE,
    .open = mpu6050_open,
    .read = mpu6050_read,
    .release = mpu6050_release,
};


static int mpu6050_probe(struct i2c_client *client, const struct i2c_device_id *id){
    int ret = 0;

    mpu6050_dev *dev = 0;
    dev = devm_kmalloc(&client->dev, sizeof(mpu6050_dev), GFP_KERNEL);

    if (dev == NULL){
        return -ENOMEM;
    }
    /* 1.创建设备号 */
    ret = alloc_chrdev_region(&(dev->dev_id), 0, MPU6050_CNT, MPU6050_NAME);
    if (ret < 0){
        pr_err("mpu6050:alloc_chrdev_region failed\n");
        return -ENOMEM;
    }
    /* 2.初始化cdev */
    dev->cdev.owner = THIS_MODULE;
    cdev_init(&(dev->cdev), &mpu6050_ops);

    /* 3.添加一个cdev */
    ret = cdev_add(&(dev->cdev), dev->dev_id, MPU6050_CNT);
    if (ret < 0){
        pr_err("mpu6050:cdev_add failed\n");
        goto del_unregister;
    }
    /* 4.创建类 */
    dev->class = class_create(THIS_MODULE, MPU6050_NAME);
    if (IS_ERR(dev->class)){
        goto del_cdev;
    }
    /* 5.创建设备 */
    dev->device = device_create(dev->class, NULL, dev->dev_id, NULL, MPU6050_NAME);
    if (IS_ERR(dev->device)){
        goto destroy_class;
    }
    dev->client = client;
    i2c_set_clientdata(client, dev);

    return 0;

destroy_class:
    device_destroy(dev->class, dev->dev_id);
del_cdev:
    cdev_del(&(dev->cdev));
del_unregister:
    unregister_chrdev_region(dev->dev_id, MPU6050_CNT);
    return -EIO;
}

static int mpu6050_remove(struct i2c_client *client){
    mpu6050_dev *dev = i2c_get_clientdata(client);

    device_destroy(dev->class, dev->dev_id);
    class_destroy(dev->class);
    cdev_del(&(dev->cdev));
    unregister_chrdev_region(dev->dev_id, MPU6050_CNT);

    return 0;
}


/* 传统匹配方式 ID 列表 */
const struct i2c_device_id mpu6050_id[] = {
    {"mpu6050", 0},
    {  }
};

const struct of_device_id mpu6050_of_match[] = {
    {.compatible = "mpu6050,learn"},
    {  }
};

MODULE_DEVICE_TABLE(of, mpu6050_of_match);

/* i2c 驱动结构体 */
static struct i2c_driver mpu6050_driver = {
    .probe = mpu6050_probe,
    .remove = mpu6050_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "mpu6050",
        .of_match_table = mpu6050_of_match,
    },
    .id_table = mpu6050_id,
};

/* 驱动入口函数 */
static int __init mpu6050_init(void)
{
    int ret = 0;
    pr_err("mpu:init \n");
    ret = i2c_add_driver(&mpu6050_driver);
    return ret;
}

/* 驱动出口函数 */
static void __exit mpu6050_exit(void)
{
    i2c_del_driver(&mpu6050_driver);
}

module_init(mpu6050_init);
module_exit(mpu6050_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SQQ");
MODULE_INFO(intree, "Y");
