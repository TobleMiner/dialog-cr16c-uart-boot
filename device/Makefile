#cr16-c-elf-gcc -mcr16c -Wall -Wextra -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes -Wundef -Wshadow -Wstrict-prototypes -Wno-unused -Werror=return-type -nostartfiles -O0 -c test.c -o test.o; \
#cr16-c-elf-ld -lgcc --gc-sections --print-memory-usage -L "$$HOME/opt/cross/lib/gcc/cr16-c-elf/10.4.0/" -T sc14441-uart.ld test.o -o test; \

SRCS=crt0.s vectors.s uart.c test.c crc32.c qspi.c system.c dma.c startup.c chipid.c

TOOLCHAIN ?= "$$HOME/opt/cross"

CC := "$(TOOLCHAIN)/bin/cr16-c-elf-gcc"
OBJCOPY := "$(TOOLCHAIN)/bin/cr16-c-elf-objcopy"

GCC_VERSION ?= "10.5.0"

all: force
	$(CC) -L "$(TOOLCHAIN)/cr16-c-elf/lib/" -I "$(TOOLCHAIN)/cr16-c-elf/include/" -mcr16c -Wall -Wextra -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes -Wundef -Wshadow -Wstrict-prototypes -Wno-unused -Werror=return-type -nostartfiles -Wl,-lgcc -Wl,--gc-sections -Wl,--print-memory-usage -Wl,-L -Wl,"$(TOOLCHAIN)/lib/gcc/cr16-c-elf/$(GCC_VERSION)/" -I "$(TOOLCHAIN)/lib/gcc/cr16-c-elf/$(GCC_VERSION)/include/" -T sc14441-uart.ld -Os -flto -ggdb $(SRCS) -o test; \
	$(OBJCOPY) -O binary test test.bin

force:

.PHONY: all force
