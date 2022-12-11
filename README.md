Very messy set of tools for working with the UART bootloader of CR16C based Dialog DECT SoCs.

dialogtool.py in [host](/host) directory supports reading SoC chip id, flash info and reading/writing to SPI flash on supported SoCs (at least SC14441).

Device side loader code lives in [device](/device) directory.  
This repo includes a prebuilt loader binary. Toolchain is not required unless you want to modify the loader binary.
Loader build infrastructure assumes cr16-c-elf toolchain is installed in `$HOME/opt/cross/` and cr16-c-elf-* binaries are in `$PATH`  
Needs libc in prefix `$HOME/opt/cross/cr16-c-elf/`  
