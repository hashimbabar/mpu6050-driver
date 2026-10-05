/*
 * Linux example (Raspberry Pi or any board with /dev/i2c-N).
 *
 * Wiring on a Raspberry Pi: VCC -> 3V3 (pin 1), GND -> pin 6,
 * SDA -> GPIO2 (pin 3), SCL -> GPIO3 (pin 5). Enable I2C in raspi-config.
 *
 * Build: make example-linux
 * Run:   ./build/mpu6050_linux /dev/i2c-1
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "mpu6050.h"

/* Register write: one message, [reg, data...]. */
static int linux_write(void *ctx, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len)
{
    int fd = *(int *)ctx;
    uint8_t buf[1 + 32];
    struct i2c_msg msg = {.addr = addr, .flags = 0, .len = (uint16_t)(len + 1), .buf = buf};
    struct i2c_rdwr_ioctl_data xfer = {.msgs = &msg, .nmsgs = 1};

    if (len > sizeof(buf) - 1) {
        return -1;
    }
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    return ioctl(fd, I2C_RDWR, &xfer) < 0 ? -1 : 0;
}

/* Register read: write the register number, then a repeated-start read. */
static int linux_read(void *ctx, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    int fd = *(int *)ctx;
    struct i2c_msg msgs[2] = {
        {.addr = addr, .flags = 0, .len = 1, .buf = &reg},
        {.addr = addr, .flags = I2C_M_RD, .len = len, .buf = data},
    };
    struct i2c_rdwr_ioctl_data xfer = {.msgs = msgs, .nmsgs = 2};

    return ioctl(fd, I2C_RDWR, &xfer) < 0 ? -1 : 0;
}

static void linux_delay(void *ctx, uint32_t ms)
{
    struct timespec ts = {.tv_sec = ms / 1000, .tv_nsec = (long)(ms % 1000) * 1000000L};
    (void)ctx;
    nanosleep(&ts, NULL);
}

int main(int argc, char **argv)
{
    const char *dev = argc > 1 ? argv[1] : "/dev/i2c-1";
    int fd = open(dev, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "open %s: %s\n", dev, strerror(errno));
        return 1;
    }

    const mpu6050_i2c_t bus = {
        .write = linux_write,
        .read = linux_read,
        .delay_ms = linux_delay,
        .ctx = &fd,
    };
    const mpu6050_config_t cfg = {
        .accel_range = MPU6050_ACCEL_4G,
        .gyro_range = MPU6050_GYRO_500DPS,
        .dlpf = MPU6050_DLPF_44HZ,
        .sample_rate_div = 9, /* 1 kHz / 10 = 100 Hz */
    };
    mpu6050_t imu;

    mpu6050_err_t err = mpu6050_init(&imu, &bus, MPU6050_ADDR_AD0_LOW, &cfg);
    if (err != MPU6050_OK) {
        fprintf(stderr, "mpu6050_init: %s\n", mpu6050_strerror(err));
        close(fd);
        return 1;
    }

    for (int i = 0; i < 20; i++) {
        mpu6050_raw_t raw;
        err = mpu6050_read_raw(&imu, &raw);
        if (err != MPU6050_OK) {
            fprintf(stderr, "read: %s\n", mpu6050_strerror(err));
            break;
        }
        printf("accel %6.2f %6.2f %6.2f g   gyro %7.1f %7.1f %7.1f dps   %.1f C\n",
               mpu6050_accel_g(&imu, raw.ax), mpu6050_accel_g(&imu, raw.ay),
               mpu6050_accel_g(&imu, raw.az), mpu6050_gyro_dps(&imu, raw.gx),
               mpu6050_gyro_dps(&imu, raw.gy), mpu6050_gyro_dps(&imu, raw.gz),
               mpu6050_temp_c(raw.temp));
        linux_delay(NULL, 100);
    }

    close(fd);
    return 0;
}
