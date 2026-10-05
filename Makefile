# mpu6050-driver
#   make test           build and run the unit tests on the PC
#   make example-linux  build the Raspberry Pi / Linux i2c-dev example

CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -O1 -g
SAN     ?= -fsanitize=address,undefined
BUILD   := build

.PHONY: test example-linux clean

test: $(BUILD)/test_mpu6050
	./$(BUILD)/test_mpu6050

$(BUILD)/test_mpu6050: src/mpu6050.c test/test_mpu6050.c test/mock_i2c.c test/unity/unity.c include/*.h | $(BUILD)
	$(CC) $(CFLAGS) $(SAN) -Iinclude -Itest -Itest/unity \
		src/mpu6050.c test/test_mpu6050.c test/mock_i2c.c test/unity/unity.c -o $@

example-linux: $(BUILD)/mpu6050_linux

$(BUILD)/mpu6050_linux: src/mpu6050.c examples/linux_i2cdev/main.c include/*.h | $(BUILD)
	$(CC) -std=gnu11 -Wall -Wextra -Werror -O2 -Iinclude src/mpu6050.c examples/linux_i2cdev/main.c -o $@

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
