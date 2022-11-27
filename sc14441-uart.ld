ENTRY(_bootloader_entrypoint)
MEMORY
{
 RES (r)	: ORIGIN = 0x00010000, LENGTH = 0x80
 ram (rwx)	: ORIGIN = 0x00010080, LENGTH = 0x3F80 /* 16K - 128 byte */
}
SECTIONS
{
 .text : {
  *(.text*)
  . = ALIGN(4);
  *(.rodata*)
 } >ram
 .preinit_array : {
  . = ALIGN(4);
  __preinit_array_start = .;
  KEEP (*(.preinit_array))
  __preinit_array_end = .;
 } >ram
 .init_array : {
  . = ALIGN(4);
  __init_array_start = .;
  KEEP (*(SORT(.init_array.*)))
  KEEP (*(.init_array))
  __init_array_end = .;
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
}
