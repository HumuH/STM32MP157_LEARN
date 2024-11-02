#ifndef _MPU6050_H_
#define _MPU6050_H_

#define MPU6050_DEVICE_ADDR 0x68

/* reg address start */
//control
#define MPU_REG_SMPLRT_DIV          0x19
#define MPU_REG_CONFIG              0x1A
#define MPU_REG_GYRO_CONFIG         0x1B
#define MPU_REG_ACCEL_CONFIG        0x1C
#define MPU_REG_FIFO_EN             0x23
#define MPU_REG_PWR_MGMT_1          0x6B
#define MPU_REG_PWR_MGMT_2          0x6C

//accel read
#define MPU_REG_ACCEL_XOUT_H        0x3B
#define MPU_REG_ACCEL_XOUT_L        0x3C
#define MPU_REG_ACCEL_YOUT_H        0x3D
#define MPU_REG_ACCEL_YOUT_L        0x3E
#define MPU_REG_ACCEL_ZOUT_H        0x3F
#define MPU_REG_ACCEL_ZOUT_L        0x40
//temp read
#define MPU_REG_TEMP_OUT_H          0x41
#define MPU_REG_TEMP_OUT_L          0x42
//gyro read
#define MPU_REG_GYRO_XOUT_H         0x43
#define MPU_REG_GYRO_XOUT_L         0x44
#define MPU_REG_GYRO_YOUT_H         0x45
#define MPU_REG_GYRO_YOUT_L         0x46
#define MPU_REG_GYRO_ZOUT_H         0x47
#define MPU_REG_GYRO_ZOUT_L         0x48
/* reg address end */

/* reg mask start */
#define MPU_MASK_PWR_MGMT_1_RESET   0x80
#define MPU_MASK_PWR_MGMT_1_SLEEP   0x40
#define MPU_MASK_PWR_MGMT_1_WAKEUP  0x00
#define MPU_MASK_PWR_MGMT_1_TEMP_DIS 0x08
#define MPU_MASK_PWR_MGMT_1_CLKSEL  0x07
#define MPU_MASK_SMPLRT_DIV_RATE    0xFF

#define MPU_MASK_GYRO_CONFIG_XG_ST  0x80
#define MPU_MASK_GYRO_CONFIG_YG_ST  0x40
#define MPU_MASK_GYRO_CONFIG_ZG_ST  0x20
#define MPU_MASK_GYRO_CONFIG_FS_SEL 0x18

#define MPU_MASK_ACCEL_CONFIG_XA_ST 0x80
#define MPU_MASK_ACCEL_CONFIG_YA_ST 0x40
#define MPU_MASK_ACCEL_CONFIG_ZA_ST 0x20
#define MPU_MASK_ACCEL_CONFIG_FS_SEL 0x18

/* reg mask end */
#endif