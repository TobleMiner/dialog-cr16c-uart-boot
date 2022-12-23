dialog-cr16c-uart-boot
=============================

### Overview
UART bootloader for CR16C based Dialog DECT SoCs

https://user-images.githubusercontent.com/748791/209271987-683d4506-6c61-477d-b1db-ebee4b655a7c.mp4

dialog-cr16c-uart-boot works similar to tools like esptool.py and will upload a loader program to RAM via the phones built-in ROM UART/serial bootloader.
The flash loader program can then be communicated with using dialogtool.py to do things like read/write to SPI flash, check chip IDs, etc.

### Required hardware

dialog-cr16c-uart-boot is designed to work with https://github.com/Manawyrm/Gigaset-Debug-Adapter#readme, altough a 1.8V USB-UART converter can also be used.
Interfacing with the test pads on the phone PCB manually while supplying the phone with power is tricky. The DECT TX pin needs to be pulled down to ground to force the phone into the UART loader.

### Tested SoCs/phones
Osmocom Wiki contains a list of tested & supported phones/SoCs:  
https://osmocom.org/projects/misc-dect-hacks/wiki/Dialog-cr16c-uart-boot

These include Gigaset C430 (Dialog SC14441, SC14448), CL660HX (Dialog SC14444), etc. phones.

### Usage

```bash
usage: dialogtool.py [-h] [-p PORT] [-b BAUDRATE] [-l LOADER] [--skip-loader]
                     [--initial-baudrate INITIAL_BAUDRATE]
                     {chip_id,flash_info,read_flash,write_flash}
```

##### Reading Chip ID (safe, good connection test)
```bash
./host/dialogtool.py -p /dev/ttyUSB0 chip_id
```

##### Dumping flash content (safe)
```bash
./host/dialogtool.py -p /dev/ttyUSB0 read_flash gigaset_c430_dump.bin
```

In some cases, the automatic flash size detection can fail.  
The flash size can be provided manually to force reading (for a 8 MiB flash chip):
```bash
./host/dialogtool.py -p /dev/ttyUSB0 read_flash gigaset_c430_dump.bin 0x0 0x800000 # [offset] [length], both decimal and hex (with 0x prefix) are supported
```

##### Writing flash content (unsafe, dangerous)
The UART bootloader cannot be bricked, but you might render your phone unbootable if you do not have a valid firmware dump (or upload a broken firmware).  
**Proceed with caution and validate you have (ideally multiple copies) of a valid firmware dump.**

```bash
./host/dialogtool.py -p /dev/ttyUSB0 write_flash gigaset_c430_dump.bin
```

Partial writing is also supported. This can be useful for development, as a full flash write can take a long time.
```bash
./host/dialogtool.py -p /dev/ttyUSB0 write_flash gigaset_c430_dump.bin 0x100000 0x1000 # [offset] [length], both decimal and hex (with 0x prefix) are supported
```

### Loader stub

Device side loader code lives in [device](/device) directory.  
This repo includes a prebuilt loader binary. Toolchain is not required unless you want to modify the loader binary.
Loader build infrastructure assumes cr16-c-elf toolchain is installed in `$HOME/opt/cross/` and cr16-c-elf-* binaries are in `$PATH`  
Needs libc in prefix `$HOME/opt/cross/cr16-c-elf/`  
