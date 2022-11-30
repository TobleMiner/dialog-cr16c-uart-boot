#cr16-c-elf-gcc -mcr16c -Wall -Wextra -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes -Wundef -Wshadow -Wstrict-prototypes -Wno-unused -Werror=return-type -nostartfiles -O0 -c test.c -o test.o; \
#cr16-c-elf-ld -lgcc --gc-sections --print-memory-usage -L "$$HOME/opt/cross/lib/gcc/cr16-c-elf/10.4.0/" -T sc14441-uart.ld test.o -o test; \

SRCS=crt0.s vectors.s uart.c test.c crc32.c qspi.c system.c dma.c startup.c

all: force
	cr16-c-elf-gcc -L "$$HOME/opt/cross/cr16-c-elf/lib/" -I "$$HOME/opt/cross/cr16-c-elf/include/" -mcr16c -Wall -Wextra -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes -Wundef -Wshadow -Wstrict-prototypes -Wno-unused -Werror=return-type -nostartfiles -Wl,-lgcc -Wl,--gc-sections -Wl,--print-memory-usage -Wl,-L -Wl,"$HOME/opt/cross/lib/gcc/cr16-c-elf/10.4.0/" -I "$HOME/opt/cross/lib/gcc/cr16-c-elf/10.4.0/include/" -T sc14441-uart.ld -Os -ggdb $(SRCS) -o test; \
	cr16-c-elf-objcopy -O binary test test.bin

force:

.PHONY: all force
