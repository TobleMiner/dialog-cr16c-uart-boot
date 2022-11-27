
#define MMIO16(addr) (*((volatile unsigned int*)addr))

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

#define WATCHDOG_REG MMIO16(0xFF4C00)

#define P0_MODE_REG MMIO16(0xFF4838)
#define P0_DIR_REG MMIO16(0xFF4836)
#define P0_DATA_REG MMIO16(0xFF4830)
#define P0_SET_DATA_REG MMIO16(0xFF4832)
#define P0_RESET_DATA_REG MMIO16(0xFF4834)

extern unsigned int _data_loadaddr;
extern unsigned int _data;
extern unsigned int _ebss;
extern unsigned int _edata;

typedef void (*funcp_t) (void);
extern funcp_t _preinit_array_start, _preinit_array_end;
extern funcp_t _init_array_start, _init_array_end;

int main(void);

void bootloader_entrypoint(void);
void bootloader_entrypoint() {
	volatile unsigned *src, *dest;
	funcp_t *fp;

	for (src = &_data_loadaddr, dest = &_data;
		dest < &_edata;
		src++, dest++) {
		*dest = *src;
	}

	while (dest < &_ebss) {
		*dest++ = 0;
	}

	/* Call constructors. */
	for (fp = &_preinit_array_start; fp < &_preinit_array_end; fp++) {
		(*fp)();
	}
	for (fp = &_init_array_start; fp < &_init_array_end; fp++) {
		(*fp)();
	}

	(void)main();
}

static void uart_puts(const char *str) {
	while (*str) {
		UART_RX_TX_REG = *str++;
		while (!(UART_CTRL_REG & UART_CTRL_REG_TI)) {
			if (UART_CTRL_REG & UART_CTRL_REG_RI) {
				UART_CLEAR_RX_INT_REG = 1;
			}
		}
		UART_CLEAR_TX_INT_REG = 1;
	}
}

int main(void) {
	P0_DIR_REG |= 0x03;
	P0_MODE_REG |= 0x01;

	UART_CTRL_REG = UART_CTRL_REG_UART_TEN | UART_CTRL_REG_UART_REN;

	while (1) {
		WATCHDOG_REG = 0xff;
		uart_puts("Hello World!\r\n");
		for (volatile unsigned int i = 0; i < 10000; i++);
/*
		P0_SET_DATA_REG = 1;
		for (volatile unsigned int i = 0; i < 10000; i++);
		P0_RESET_DATA_REG = 1;
*/
//		for (volatile unsigned int i = 0; i < 10000; i++);
	}
}
