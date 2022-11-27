#cr16-c-elf-gcc -mcr16c -Wall -Wextra -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes -Wundef -Wshadow -Wstrict-prototypes -Wno-unused -Werror=return-type -nostartfiles -O0 -c test.c -o test.o; \
#cr16-c-elf-ld -lgcc --gc-sections --print-memory-usage -L "$$HOME/opt/cross/lib/gcc/cr16-c-elf/10.4.0/" -T sc14441-uart.ld test.o -o test; \

all: force
	cr16-c-elf-gcc -nostdlib -mcr16c -Wall -Wextra -Wimplicit-function-declaration -Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes -Wundef -Wshadow -Wstrict-prototypes -Wno-unused -Werror=return-type -nostartfiles -O0 -Wl,-lgcc -Wl,--gc-sections -Wl,--print-memory-usage -Wl,-L -Wl,"$HOME/opt/cross/lib/gcc/cr16-c-elf/10.4.0/" -T sc14441-uart.ld test.c -o test; \
	cr16-c-elf-objcopy -O binary test test.bin

force:

.PHONY: all force
