# mpu6050-driver

[![ci](https://github.com/hashimbabar/mpu6050-driver/actions/workflows/ci.yml/badge.svg)](https://github.com/hashimbabar/mpu6050-driver/actions/workflows/ci.yml)

A small C driver for the InvenSense MPU-6050 6-axis IMU, written from the
register map. It has no hardware dependencies. You give it three functions
(I2C write, I2C read, delay), so the same code runs on an STM32, a Raspberry
Pi, or a PC with a mocked bus for unit tests.

I wrote it for [ShotTrack](https://github.com/hashimbabar/shot-track), a
wrist-worn basketball shot counter, and split it out because it's useful on
its own.

**Status:** unit-tested against a mocked I2C bus, and compiles for Cortex-M4
and Linux. Not yet tested against a physical MPU-6050 (my hardware hasn't
arrived). Issues and fixes from real boards are welcome.

## Features

- WHO_AM_I check before anything is written
- Reset, wake from sleep, gyro PLL clock
- Accel range (±2/4/8/16 g), gyro range (±250/500/1000/2000 °/s), digital
  low-pass filter and sample-rate divider
- One 14-byte burst read for accel + temperature + gyro, so all axes come
  from the same sample
- Conversions to g, °/s and °C
- Error codes: `MPU6050_ERR_I2C` (NACK/bus error), `MPU6050_ERR_WHO_AM_I`,
  `MPU6050_ERR_PARAM`
- No malloc, no globals, about 600 bytes of code on a Cortex-M4 (`-Os`)
- Every register in `mpu6050_regs.h` is commented with its section in the
  register map (RM-MPU-6000A-00 rev 4.2)

## Usage

```c
#include "mpu6050.h"

/* 1. Your platform's I2C functions. Return 0 on success. */
static int my_write(void *ctx, uint8_t addr, uint8_t reg, const uint8_t *data, uint16_t len);
static int my_read(void *ctx, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
static void my_delay(void *ctx, uint32_t ms);

static const mpu6050_i2c_t bus = {my_write, my_read, my_delay, NULL};

/* 2. Configure. */
const mpu6050_config_t cfg = {
    .accel_range = MPU6050_ACCEL_8G,
    .gyro_range = MPU6050_GYRO_1000DPS,
    .dlpf = MPU6050_DLPF_44HZ,
    .sample_rate_div = 4,          /* 1 kHz / (1 + 4) = 200 Hz */
};
mpu6050_t imu;

if (mpu6050_init(&imu, &bus, MPU6050_ADDR_AD0_LOW, &cfg) != MPU6050_OK) {
    /* check wiring; mpu6050_strerror(err) says what failed */
}

/* 3. Read. */
mpu6050_raw_t raw;
if (mpu6050_read_raw(&imu, &raw) == MPU6050_OK) {
    float az_g = mpu6050_accel_g(&imu, raw.az);
    float gx_dps = mpu6050_gyro_dps(&imu, raw.gx);
}
```

Complete examples:

- [`examples/stm32_hal/mpu6050_stm32.c`](examples/stm32_hal/mpu6050_stm32.c):
  STM32 HAL (`HAL_I2C_Mem_Read/Write`)
- [`examples/linux_i2cdev/main.c`](examples/linux_i2cdev/main.c): Linux
  `/dev/i2c-N` with `I2C_RDWR`, e.g. a Raspberry Pi. Build with
  `make example-linux`

## Adding it to a project

Copy `include/` and `src/` into your project, or with PlatformIO:

```ini
lib_deps = https://github.com/hashimbabar/mpu6050-driver.git
```

## Tests

```
make test     # 15 Unity tests, built with gcc and run with address/UB sanitizers
make cross    # compile for Cortex-M4 (needs arm-none-eabi-gcc)
```

The tests use a fake I2C bus (`test/mock_i2c.c`) with a 128-byte register
file. It records every write in order and can NACK a chosen transaction.
They check:

- the exact init write sequence (register and value, in order)
- WHO_AM_I is read first, and nothing is written if it's wrong
- a NACK at the start or partway through init returns `MPU6050_ERR_I2C`
  and stops
- the burst read is a single 14-byte read from 0x3B
- big-endian, signed parsing, including -32768, -1 and 32767
- scale factors and the temperature formula

Unity (MIT, ThrowTheSwitch.org) is vendored in `test/unity/`.

## License

MIT, see [LICENSE](LICENSE).
