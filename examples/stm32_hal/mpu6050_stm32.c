/*
 * STM32 HAL example: the three bus functions for an STM32 I2C peripheral,
 * and how to use the driver from your own code.
 *
 * Not a complete project: drop this file into an STM32Cube/PlatformIO
 * project that already initialises hi2c1 (400 kHz is fine), and call
 * imu_example() after HAL_Init() and your clock/I2C setup.
 *
 * This is the same adapter the ShotTrack firmware uses, minus FreeRTOS
 * (HAL_Delay instead of vTaskDelay).
 */
#include <stdio.h>
#include "main.h"     /* your project's HAL includes; defines hi2c1 */
#include "mpu6050.h"

extern I2C_HandleTypeDef hi2c1;

#define I2C_TIMEOUT_MS 5

/* The HAL wants the 7-bit address shifted left by one. */
static int hal_write(void *ctx, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Write((I2C_HandleTypeDef *)ctx, (uint16_t)(addr << 1), reg,
                             I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, len, I2C_TIMEOUT_MS) == HAL_OK ? 0 : -1;
}

static int hal_read(void *ctx, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Read((I2C_HandleTypeDef *)ctx, (uint16_t)(addr << 1), reg,
                            I2C_MEMADD_SIZE_8BIT, data, len, I2C_TIMEOUT_MS) == HAL_OK ? 0 : -1;
}

static void hal_delay(void *ctx, uint32_t ms)
{
    (void)ctx;
    HAL_Delay(ms);
}

static const mpu6050_i2c_t imu_bus = {
    .write = hal_write,
    .read = hal_read,
    .delay_ms = hal_delay,
    .ctx = &hi2c1,
};

void imu_example(void)
{
    const mpu6050_config_t cfg = {
        .accel_range = MPU6050_ACCEL_8G,
        .gyro_range = MPU6050_GYRO_1000DPS,
        .dlpf = MPU6050_DLPF_44HZ,
        .sample_rate_div = 4, /* 200 Hz */
    };
    mpu6050_t imu;
    mpu6050_raw_t raw;

    mpu6050_err_t err = mpu6050_init(&imu, &imu_bus, MPU6050_ADDR_AD0_LOW, &cfg);
    if (err != MPU6050_OK) {
        printf("mpu6050_init: %s\r\n", mpu6050_strerror(err));
        return;
    }

    for (;;) {
        if (mpu6050_read_raw(&imu, &raw) == MPU6050_OK) {
            /* Integer milli-units, since newlib-nano's printf has no %f by default. */
            printf("az %ld mg, gx %ld mdps\r\n",
                   (long)(mpu6050_accel_g(&imu, raw.az) * 1000.0f),
                   (long)(mpu6050_gyro_dps(&imu, raw.gx) * 1000.0f));
        }
        HAL_Delay(100);
    }
}
