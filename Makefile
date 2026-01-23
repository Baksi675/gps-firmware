CC := arm-none-eabi-gcc

INCLUDES = -Iconfiguration -Icore/inc -Idrivers/inc

CFLAGS := -c -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -std=gnu11 -Wall -O0 -g $(INCLUDES)
LDFLAGS := -mcpu=cortex-m4 -mthumb --specs=nano.specs -nostartfiles -T stm32f401re.ld -Wl,-Map=build/output.map

.PHONY: clean build

build/output.elf: build/startup_stm32f401re.o \
					build/main.o \
					build/led.o \
					build/stm32f401re_rcc.o \
					| build
	$(CC) $(LDFLAGS) $^ -o $@

build/startup_stm32f401re.o: startup/startup_stm32f401re.c | build
	$(CC) $(CFLAGS) $< -o $@

build/main.o: core/src/main.c | build
	$(CC) $(CFLAGS) $< -o $@

build/led.o: core/src/led.c | build
	$(CC) $(CFLAGS) $< -o $@

build/stm32f401re_rcc.o: drivers/src/stm32f401re_rcc.c | build
	$(CC) $(CFLAGS) $< -o $@

### PHONY ###

clean:
	rm -rf build

build:
	mkdir -p build