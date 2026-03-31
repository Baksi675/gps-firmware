CC := arm-none-eabi-gcc

INCLUDES = -Iconfiguration -Icore/inc -Idrivers/inc

CFLAGS := -c -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -std=gnu11 -Wall -O0 -g $(INCLUDES)
LDFLAGS := -mcpu=cortex-m4 -mthumb --specs=nano.specs -nostartfiles -T stm32f401re.ld -Wl,-Map=build/output.map

.PHONY: clean build

build/output.elf: build/startup_stm32f401re.o \
					build/main.o \
					build/io.o \
					build/stm32f401re_rcc.o \
					build/arm_cortex_m4_nvic.o \
					build/stm32f401re_usart.o \
					build/common.o \
					build/cbuf.o \
					build/stm32f401re_gpio.o \
					build/console.o \
					build/stm32f401re_rtc.o \
					build/log.o \
					build/cmd.o \
					build/modules.o \
					build/stm32f401re_i2c.o \
					build/ssd1309.o \
					build/button.o \
					build/arm_cortex_m4_systick.o \
					build/menu.o \
					build/neo6.o \
					build/stm32f401re_spi.o \
					build/sd.o \
					build/ff.o \
					build/diskio.o \
					build/syscalls.o \
					build/datalog.o \
					build/init.o \
					| build
	$(CC) $(LDFLAGS) $^ -o $@

build/startup_stm32f401re.o: startup/startup_stm32f401re.c | build
	$(CC) $(CFLAGS) $< -o $@

build/main.o: core/src/main.c | build
	$(CC) $(CFLAGS) $< -o $@

build/io.o: core/src/io.c | build
	$(CC) $(CFLAGS) $< -o $@

build/stm32f401re_rcc.o: drivers/src/stm32f401re_rcc.c | build
	$(CC) $(CFLAGS) $< -o $@

build/arm_cortex_m4_nvic.o: drivers/src/arm_cortex_m4_nvic.c | build
	$(CC) $(CFLAGS) $< -o $@

build/stm32f401re_usart.o: drivers/src/stm32f401re_usart.c | build
	$(CC) $(CFLAGS) $< -o $@

build/common.o: core/src/common.c | build
	$(CC) $(CFLAGS) $< -o $@

build/cbuf.o: core/src/cbuf.c | build
	$(CC) $(CFLAGS) $< -o $@

build/stm32f401re_gpio.o: drivers/src/stm32f401re_gpio.c | build
	$(CC) $(CFLAGS) $< -o $@

build/console.o: core/src/console.c | build
	$(CC) $(CFLAGS) $< -o $@

build/stm32f401re_rtc.o: drivers/src/stm32f401re_rtc.c | build
	$(CC) $(CFLAGS) $< -o $@

build/log.o: core/src/log.c | build
	$(CC) $(CFLAGS) $< -o $@

build/cmd.o: core/src/cmd.c | build
	$(CC) $(CFLAGS) $< -o $@

build/modules.o: core/src/modules.c | build
	$(CC) $(CFLAGS) $< -o $@

build/stm32f401re_i2c.o: drivers/src/stm32f401re_i2c.c | build
	$(CC) $(CFLAGS) $< -o $@

build/ssd1309.o: core/src/ssd1309.c | build
	$(CC) $(CFLAGS) $< -o $@

build/button.o: core/src/button.c | build
	$(CC) $(CFLAGS) $< -o $@

build/arm_cortex_m4_systick.o: drivers/src/arm_cortex_m4_systick.c | build
	$(CC) $(CFLAGS) $< -o $@

build/menu.o: core/src/menu.c | build
	$(CC) $(CFLAGS) $< -o $@

build/neo6.o: core/src/neo6.c | build
	$(CC) $(CFLAGS) $< -o $@

build/stm32f401re_spi.o: drivers/src/stm32f401re_spi.c | build
	$(CC) $(CFLAGS) $< -o $@

build/sd.o: core/src/sd.c | build
	$(CC) $(CFLAGS) $< -o $@

build/ff.o: core/src/ff.c | build
	$(CC) $(CFLAGS) $< -o $@

build/diskio.o: core/src/diskio.c | build
	$(CC) $(CFLAGS) $< -o $@

build/syscalls.o: core/src/syscalls.c | build
	$(CC) $(CFLAGS) $< -o $@

build/datalog.o: core/src/datalog.c | build
	$(CC) $(CFLAGS) $< -o $@

build/init.o: core/src/init.c | build
	$(CC) $(CFLAGS) $< -o $@

### PHONY ###

clean:
	rm -rf build

build:
	mkdir -p build