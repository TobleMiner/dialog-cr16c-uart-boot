#include "uart.h"

#include "gpio.h"
#include "irq.h"
#include "watchdog.h"

void uart_init(void) {
	P0_DIR_REG |= 0x03;
	P0_MODE_REG |= 0x01;
	UART_CTRL_REG = UART_CTRL_REG_UART_TEN | UART_CTRL_REG_UART_REN;

//	INT0_PRIORITY_REG = 0x2222;
//	INT1_PRIORITY_REG = 0x2222;
	INT2_PRIORITY_REG = 0x0002;
//	INT3_PRIORITY_REG = 0x2222;
/*
	INT2_PRIORITY_REG &= INT2_PRIORITY_REG_UART_RI_INT_PRIO_MASK;
	INT2_PRIORITY_REG |= (4 << INT2_PRIORITY_REG_UART_RI_INT_PRIO_SHIFT);
*/
}

void uart_set_baudrate(unsigned long baudrate) {
	unsigned int uart_enable = UART_CTRL_REG & (UART_CTRL_REG_UART_TEN | UART_CTRL_REG_UART_REN);
	UART_CTRL_REG &= ~(UART_CTRL_REG_UART_TEN | UART_CTRL_REG_UART_REN);
	unsigned int uart_ctrl = UART_CTRL_REG;
	uart_ctrl &= ~UART_CTRL_REG_BAUDRATE_MASK;
	switch (baudrate) {
	case 9600UL:
		uart_ctrl |= UART_CTRL_REG_BAUDRATE_9600;
		break;
	case 19200UL:
		uart_ctrl |= UART_CTRL_REG_BAUDRATE_19200;
		break;
	case 57600UL:
		uart_ctrl |= UART_CTRL_REG_BAUDRATE_57600;
		break;
	case 115200UL:
		uart_ctrl |= UART_CTRL_REG_BAUDRATE_115200;
		break;
	case 230400UL:
		uart_ctrl |= UART_CTRL_REG_BAUDRATE_230400;
		break;
	}
	UART_CTRL_REG = uart_ctrl;
	UART_CTRL_REG |= uart_enable;
}

void uart_putc(char c) {
	UART_RX_TX_REG = c;
	while (!(UART_CTRL_REG & UART_CTRL_REG_TI));
	UART_CLEAR_TX_INT_REG = 1;
}

void uart_puts(const char *str) {
	while (*str) {
		watchdog_reset();
		uart_putc(*str++);
	}
}

#define NIBBLE_TO_HEX_CHAR(i) ((i) < 10 ? '0' + (i) : 'A' + ((i) - 10))

void uart_putbyte_hex(unsigned char byt) {
	char str[3];
	str[0] = NIBBLE_TO_HEX_CHAR((byt >> 4) & 0xf);
	str[1] = NIBBLE_TO_HEX_CHAR(byt & 0xf);
	str[2] = 0;
	uart_puts(str);
}

void uart_hexdump(void *ptr, unsigned int len) {
	unsigned char *ptr8 = ptr;
	while (len--) {
		watchdog_reset();
		uart_putbyte_hex(*ptr8++);
	}
}

void uart_putint(unsigned int val) {
	char str[6];
	str[4] = '0';
	str[5] = 0;
	char *ptr = &str[4];
	for (int i = 4; i >= 0; i--) {
		if (!val) {
			break;
		}
		str[i] = '0' + (val % 10);
		ptr = &str[i];
		val /= 10;
	}
	uart_puts(ptr);
}

void uart_putlong(unsigned long val) {
	char str[11];
	str[9] = '0';
	str[10] = 0;
	char *ptr = &str[9];
	for (int i = 9; i >= 0; i--) {
		if (!val) {
			break;
		}
		str[i] = '0' + (val % 10);
		ptr = &str[i];
		val /= 10;
	}
	uart_puts(ptr);
}

void uart_putint_hex(unsigned int i) {
	uart_putbyte_hex(i >> 8);
	uart_putbyte_hex(i & 0xff);
}

void uart_putlong_hex(unsigned long i) {
	uart_putint_hex(i >> 16);
	uart_putint_hex(i & 0xffff);
}

void uart_putnewline(void) {
	uart_puts("\r\n");
}

void uart_write(const void *ptr, unsigned int len) {
	const unsigned char *ptr8 = ptr;
	while (len--) {
		watchdog_reset();
		uart_putc((char)*ptr8++);
	}
}
