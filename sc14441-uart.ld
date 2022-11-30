EXTERN(_vector_table)
ENTRY(bootloader_entry)
MEMORY
{
 RES (r)	: ORIGIN = 0x00010000, LENGTH = 0x80
 ram (rwx)	: ORIGIN = 0x00010080, LENGTH = 0x3F80 /* 16K - 128 byte */
}
SECTIONS
{
 .text : {
  *(.text.crt0)
  . = ALIGN(4);
  *(.text*)
  . = ALIGN(4);
  *(.rodata*)
  . = ALIGN(4);
  __intbase = .;
  KEEP (*(.vectors))
  . = ALIGN(4);
  __ctors_start = .;
  KEEP (*(.ctors))
  __ctors_end = .;
 } >ram
 . = ALIGN(4);
 __etext = .;
 .noinit (NOLOAD) : {
  *(.noinit*)
 } >ram
 . = ALIGN(4);
 .data : {
  __data = .;
  *(.data*)
  *(.ramtext*)
  . = ALIGN(4);
  __edata = .;
 } >ram
 __data_loadaddr = LOADADDR(.data);
 .bss : {
  *(.bss*)
  *(COMMON)
  . = ALIGN(4);
  __ebss = .;
 } >ram
 end = .;
 __istack = ORIGIN(ram) + LENGTH(ram);
 __ustack = __istack - 0x100;
}
