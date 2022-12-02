#pragma once

#include "util.h"

#define UART_CTRL_REG			MMIO16(0xFF4900)
#define UART_CTRL_REG_INV_UTX		(1 << 10)
#define UART_CTRL_REG_INV_URX		(1 << 9)
#define UART_CTRL_REG_IRDA_EN		(1 << 8)
#define UART_CTRL_REG_UART_MODE		(1 << 7)
#define UART_CTRL_REG_RI		(1 << 6)
#define UART_CTRL_REG_TI		(1 << 5)
#define UART_CTRL_REG_BAUDRATE_MASK	(7 << 2)
#define UART_CTRL_REG_BAUDRATE_9600	(0 << 2)
#define UART_CTRL_REG_BAUDRATE_19200	(1 << 2)
#define UART_CTRL_REG_BAUDRATE_57600	(2 << 2)
#define UART_CTRL_REG_BAUDRATE_115200	(3 << 2)
#define UART_CTRL_REG_BAUDRATE_230400	(4 << 2)
#define UART_CTRL_REG_UART_TEN		(1 << 1)
#define UART_CTRL_REG_UART_REN		(1 << 0)

#define UART_RX_TX_REG MMIO16(0xFF4902)
#define UART_CLEAR_TX_INT_REG MMIO16(0xFF4904)
#define UART_CLEAR_RX_INT_REG MMIO16(0xFF4906)

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *str);
void uart_putbyte_hex(unsigned char byt);
void uart_hexdump(void *ptr, unsigned int len);
void uart_putint(unsigned int val);
void uart_putlong(unsigned long val);
void uart_putint_hex(unsigned int i);
void uart_putlong_hex(unsigned long i);
void uart_putnewline(void);
void uart_write(const void *ptr, unsigned int len);

