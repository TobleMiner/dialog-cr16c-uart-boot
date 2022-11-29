#include <stddef.h>
#include <stdint.h>

#define MMIO16(addr) (*((volatile uint16_t*)(addr)))
#define MMIO32(addr) (*((volatile uint32_t*)(addr)))
#define MMIO8(addr) (*((volatile uint8_t*)(addr)))

#define DEBUG_REG MMIO16(0xFF5004)
#define DEBUG_REG_SW_RESET (1 << 7)

#define CLK_PER10_DIV_REG		MMIO16(0xFF400E)
#define CLK_PER10_DIV_REG_QSPI_DIV_MASK	(3 << 8)
#define CLK_PER10_DIV_REG_QSPI_DIV_1	(1 << 8)
#define CLK_PER10_DIV_REG_QSPI_DIV_2	(2 << 8)
#define CLK_PER10_DIV_REG_QSPI_EN	(1 << 10)

#define CLK_AMBA_REG			MMIO16(0xFF4000)
#define CLK_AMBA_REG_HCLK_DIV_MASK	(7 << 0)
#define CLK_AMBA_REG_HCLK_DIV_8		(0 << 0)
#define CLK_AMBA_REG_HCLK_DIV_1		(1 << 0)
#define CLK_AMBA_REG_HCLK_DIV_2		(2 << 0)
#define CLK_AMBA_REG_HCLK_DIV_3		(3 << 0)
#define CLK_AMBA_REG_HCLK_DIV_4		(4 << 0)
#define CLK_AMBA_REG_HCLK_DIV_5		(5 << 0)
#define CLK_AMBA_REG_HCLK_DIV_6		(6 << 0)
#define CLK_AMBA_REG_HCLK_DIV_7		(7 << 0)
#define CLK_AMBA_REG_PCLK_DIV_MASK	(3 << 3)
#define CLK_AMBA_REG_PCLK_DIV_4		(0 << 3)
#define CLK_AMBA_REG_PCLK_DIV_1		(1 << 3)
#define CLK_AMBA_REG_PCLK_DIV_2		(2 << 3)
#define CLK_AMBA_REG_SRAM1_EN		(1 << 6)

#define CLK_AUX2_REG MMIO16(0xFF402E)
#define CLK_AUX2_REG_SW_QSPIC_CLK_DIV		(1 << 13)
#define CLK_AUX2_REG_SW_QSPIC_EN		(1 << 10)
#define CLK_AUX2_REG_SW_QSPIC_SEL_SEOURCE	(1 << 9)

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

#define P0_DATA_REG MMIO16(0xFF4830)
#define P0_SET_DATA_REG MMIO16(0xFF4832)
#define P0_RESET_DATA_REG MMIO16(0xFF4834)
#define P0_DIR_REG MMIO16(0xFF4836)
#define P0_MODE_REG MMIO16(0xFF4838)

#define P1_DATA_REG MMIO16(0xFF4840)
#define P1_SET_DATA_REG MMIO16(0xFF4842)
#define P1_RESET_DATA_REG MMIO16(0xFF4844)
#define P1_DIR_REG MMIO16(0xFF4846)
#define P1_MODE_REG MMIO16(0xFF4848)

#define P2_DATA_REG MMIO16(0xFF4850)
#define P2_SET_DATA_REG MMIO16(0xFF4852)
#define P2_RESET_DATA_REG MMIO16(0xFF4854)
#define P2_DIR_REG MMIO16(0xFF4856)
#define P2_MODE_REG MMIO16(0xFF4858)

#define P3_DATA_REG MMIO16(0xFF4860)
#define P3_SET_DATA_REG MMIO16(0xFF4862)
#define P3_RESET_DATA_REG MMIO16(0xFF4864)
#define P3_DIR_REG MMIO16(0xFF4866)
#define P3_MODE_REG MMIO16(0xFF4868)

#define P4_DATA_REG MMIO16(0xFF4870)
#define P4_SET_DATA_REG MMIO16(0xFF4872)
#define P4_RESET_DATA_REG MMIO16(0xFF4874)
#define P4_DIR_REG MMIO16(0xFF4876)
#define P4_MODE_REG MMIO16(0xFF4878)

#define P5_DATA_REG MMIO16(0xFF4880)
#define P5_SET_DATA_REG MMIO16(0xFF4882)
#define P5_RESET_DATA_REG MMIO16(0xFF4884)
#define P5_DIR_REG MMIO16(0xFF4886)
#define P5_MODE_REG MMIO16(0xFF4888)

#define PORT_BASE(i)		(0xFF4830L + ((unsigned long)i) * 0x10L)
#define PORT_DATA_REG(port)	MMIO16(PORT_BASE(port) + 0x00)
#define PORT_SET_REG(port)	MMIO16(PORT_BASE(port) + 0x02)
#define PORT_RESET_REG(port)	MMIO16(PORT_BASE(port) + 0x04)
#define PORT_DIR_REG(port)	MMIO16(PORT_BASE(port) + 0x06)
#define PORT_MODE_REG(port)	MMIO16(PORT_BASE(port) + 0x08)
/*
#define QSPIC_CTRLBUS_REG MMIO32(0xFF0E00)
#define QSPIC_CTRLMODE_REG MMIO32(0xFF0E04)
#define QSPIC_RECVDATA_REG MMIO32(0xFF0E08)
#define QSPIC_BURSTCMDA_REG MMIO32(0xFF0E0C)
#define QSPIC_BURSTCMDB_REG MMIO32(0xFF0E10)
#define QSPIC_STATUS_REG MMIO32(0xFF0E14)
#define QSPIC_WRITEDATA8_REG MMIO8(0xFF0E18)
#define QSPIC_WRITEDATA16_REG MMIO16(0xFF0E18)
#define QSPIC_WRITEDATA32_REG MMIO32(0xFF0E18)
#define QSPIC_READDATA8_REG MMIO8(0xFF0E1C)
#define QSPIC_READDATA16_REG MMIO16(0xFF0E1C)
#define QSPIC_READDATA32_REG MMIO32(0xFF0E1C)
#define QSPIC_DUMMYDATA8_REG MMIO8(0xFF0E20)
#define QSPIC_DUMMYDATA16_REG MMIO16(0xFF0E20)
#define QSPIC_DUMMYDATA32_REG MMIO32(0xFF0E20)
*/
#define QSPIC_CTRL_REG16		MMIO32(0xFF0C00)
//#define QSPIC_CTRL_REG32		MMIO32(0xFF0C00)
#define QSPIC_CTRL_REG_DISABLE_BUS	(1 << 4)
#define QSPIC_CTRL_REG_ENABLE_BUS	(1 << 3)
#define QSPIC_CTRL_REG_QIO_EN		(1 << 2)
#define QSPIC_CTRL_REG_DIO_EN		(1 << 1)
#define QSPIC_CTRL_REG_SIO_EN		(1 << 0)
#define QSPIC_CFG_REG			MMIO16(0xFF0C04)
#define QSPIC_CFG_REG_UNKNOWN		(1 << 7)
#define QSPIC_CFG_REG_IO3_RST_DATA	(1 << 5)
#define QSPIC_CFG_REG_IO2_WP_DATA	(1 << 4)
#define QSPIC_CFG_REG_IO3_RST_OEN	(1 << 3)
#define QSPIC_CFG_REG_IO2_WP_OEN	(1 << 2)
#define QSPIC_CFG_REG_DO_CLK_IDLE_LEVEL	(1 << 1)
#define QSPIC_CFG_REG_ENABLE_MMIO_XIP	(1 << 0)
#define QSPIC_RECVDATA_REG		MMIO32(0xFF0C08)
#define QSPIC_STATUS_REG		MMIO16(0xFF0C14)
#define QSPIC_STATUS_REG_BUSY		(1 << 0)
#define QSPIC_WRITEDATA8_REG		MMIO8(0xFF0C18)
#define QSPIC_WRITEDATA16_REG		MMIO16(0xFF0C18)
#define QSPIC_WRITEDATA32_REG		MMIO32(0xFF0C18)
#define QSPIC_READDATA8_REG		MMIO8(0xFF0C1C)
#define QSPIC_READDATA16_REG		MMIO16(0xFF0C1C)
#define QSPIC_READDATA32_REG		MMIO32(0xFF0C1C)
#define QSPIC_UNKNOWN_REG1		MMIO16(0xFF4814)
#define QSPIC_UNKNOWN_REG2		MMIO16(0xFF481C)
#define QSPIC_UNKNOWN_REG3		MMIO16(0xFF4816)

#define QSPIC_DEASSERT_CS()	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_DISABLE_BUS

#define QSPIC_WAIT_NOT_BUSY()	do { } while (QSPIC_STATUS_REG & QSPIC_STATUS_REG_BUSY)

extern unsigned int _data_loadaddr;
extern unsigned int _data;
extern unsigned int _ebss;
extern unsigned int _edata;
extern unsigned int _stack;

typedef void (*funcp_t) (void);
extern funcp_t _preinit_array_start, _preinit_array_end;
extern funcp_t _init_array_start, _init_array_end;

int main(void);

void bootloader_entrypoint(void) __attribute__((noreturn));
void bootloader_entrypoint() {
	asm volatile ("movd $__stack, (sp) \n"
		      "movd $__stack, (r1, r0) \n"
		      "lprd (r1, r0), isp" ::: "r0", "r1");

	volatile unsigned *src, *dest;
		for (src = &_data_loadaddr, dest = &_data;
		dest < &_edata;
		src++, dest++) {
		*dest = *src;
	}

	while (dest < &_ebss) {
		*dest++ = 0;
	}

	/* Call constructors. */
	funcp_t *fp;
	for (fp = &_preinit_array_start; fp < &_preinit_array_end; fp++) {
		(*fp)();
	}
	for (fp = &_init_array_start; fp < &_init_array_end; fp++) {
		(*fp)();
	}

	(void)main();

	while (1);
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

#define NIBBLE_TO_HEX_CHAR(i) ((i) < 10 ? '0' + (i) : 'A' + ((i) - 10))

static void uart_putbyte_hex(unsigned char byt) {
	char str[3];
	str[0] = NIBBLE_TO_HEX_CHAR((byt >> 4) & 0xf);
	str[1] = NIBBLE_TO_HEX_CHAR(byt & 0xf);
	str[2] = 0;
	uart_puts(str);
}

static void uart_hexdump(void *ptr, unsigned int len) {
	unsigned char *ptr8 = ptr;
	while (len--) {
		WATCHDOG_REG = 0xff;
		uart_putbyte_hex(*ptr8++);
	}
}

static void uart_putint(unsigned int val) {
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

static void uart_putint_hex(unsigned int i) {
	uart_putbyte_hex(i >> 8);
	uart_putbyte_hex(i & 0xff);
}

static void uart_putlong_hex(unsigned long i) {
	uart_putint_hex(i >> 16);
	uart_putint_hex(i & 0xffff);
}

__attribute__((noreturn))
static void reset(void) {
	DEBUG_REG |= DEBUG_REG_SW_RESET;
	while(1);
}

static void dump_reg16(const char *name, unsigned int value) {
	uart_puts(name);
	uart_puts(": 0x");
	uart_putint_hex(value);
	uart_puts("\r\n");
}

static void dump_reg32(const char *name, unsigned long value) {
	uart_puts(name);
	uart_puts(": 0x");
	uart_putlong_hex(value);
	uart_puts("\r\n");
}

static void toggle_pin(unsigned int port, unsigned int pin) {
/*
	uart_puts("Port ");
	uart_putint(port);
	uart_puts(" direction register is @0x");
	uart_putlong_hex((unsigned long)&PORT_DIR_REG(port));
	uart_puts("\r\n");
*/
	unsigned int initial_port_data = PORT_DATA_REG(port);
	unsigned int initial_port_mode = PORT_MODE_REG(port);
	unsigned int initial_port_dir = PORT_DIR_REG(port);

	uart_puts("Toggling ");
	uart_putint(port);
	uart_puts(".");
	uart_putint(pin);
	uart_puts("\r\n");

	unsigned int new_port_mode = initial_port_mode & ~(3 << (pin * 2));
	unsigned int new_port_dir = initial_port_dir | (3 << (pin * 2));
	unsigned int new_port_data1 = initial_port_data | (1 << pin);
	unsigned int new_port_data0 = initial_port_data & ~(1 << pin);

//	PORT_MODE_REG(port) = new_port_mode;
/*
	uart_puts("Port dir: 0x");
	uart_putint_hex(new_port_dir);
	uart_puts("\r\n");
*/
	PORT_DIR_REG(port) = new_port_dir;
	PORT_DATA_REG(port) = new_port_data1;

	for (volatile unsigned int i = 0; i < 1000; i++);

	PORT_DATA_REG(port) = new_port_data0;

	for (volatile unsigned int i = 0; i < 1000; i++);

//	PORT_MODE_REG(port) = initial_port_mode;
	PORT_DIR_REG(port) = initial_port_dir;
	PORT_DATA_REG(port) = initial_port_data;
}

typedef struct port_pin {
	unsigned int port;
	int pin;
} port_pin_t;

#define ARRAY_SIZE(arr_) (sizeof(arr_) / sizeof *(arr_))

static const port_pin_t pin_blacklist[] = {
	{ 0, 0 }, // UTX
//	{ 0, 1 }, // URX
/*
	{ 3, 1 }, // Function unclear, toggling crashes device
	{ 3, 2 }, // Function unclear, toggling crashes device
	{ 3, 3 }, // Function unclear, toggling crashes device
	{ 3, -1 }, // Function unclear, do not probe any of port 3
	{ 4, -1 }, // Function unclear, do not probe any of port 3
*/
};

typedef enum {
	QSPI_MODE_SIO,
	QSPI_MODE_DIO,
	QSPI_MODE_QIO
} qspi_mode_t;

typedef struct qspi_xfer_desc {
	qspi_mode_t mode;
	const void *tx_data;
	size_t tx_len;
	size_t dummy_cycles_after_tx;
	void *rx_data;
	size_t rx_len;
} qspi_xfer_desc_t;

static void qspi_tx(const qspi_xfer_desc_t *desc) {
	const uint8_t *data = desc->tx_data;
	size_t data_len = desc->tx_len;
	size_t dummy_len = desc->dummy_cycles_after_tx;
	switch (desc->mode) {
	case QSPI_MODE_DIO:
		dummy_len *= 2;
		break;
	case QSPI_MODE_QIO:
		dummy_len *= 4;
		break;
	default:
		break;
	}

	while (data_len || dummy_len) {
		size_t combined_len = data_len + dummy_len;

		if (combined_len >= 4) {
			uint32_t datum = 0xffffffff;
			int i;
			for (i = 0; i < 4 && data_len; i++) {
				datum &= ~(((uint32_t)*data++) << (i * 8));
				data_len--;
			}
			dummy_len -= 4 - i;
			QSPIC_WAIT_NOT_BUSY();
			QSPIC_WRITEDATA32_REG = datum;
		} else if (combined_len >= 2) {
			uint16_t datum = 0xffff;
			int i;
			for (i = 0; i < 2 && data_len; i++) {
				datum &= ~(((uint16_t)*data++) << (i * 8));
				data_len--;
			}
			dummy_len -= 2 - i;
			QSPIC_WAIT_NOT_BUSY();
			QSPIC_WRITEDATA16_REG = datum;
		} else {
			QSPIC_WAIT_NOT_BUSY();
			if (data_len) {
				QSPIC_WRITEDATA8_REG = *data++;
			} else {
				QSPIC_WRITEDATA8_REG = 0xff;
			}
		}
	}
	QSPIC_WAIT_NOT_BUSY();
}

static void qspi_rx(const qspi_xfer_desc_t *desc) {
	uint8_t *data = desc->rx_data;
	size_t data_len = desc->rx_len;

	while (data_len >= 4) {
		QSPIC_READDATA32_REG;
		QSPIC_WAIT_NOT_BUSY();
		uint32_t datum = QSPIC_RECVDATA_REG;
		data[0] = (uint8_t)datum;
		data[1] = (uint8_t)(datum >> 8);
		data[2] = (uint8_t)(datum >> 16);
		data[3] = (uint8_t)(datum >> 24);
		data += 4;
		data_len -= 4;
	}

	while (data_len >= 2) {
		QSPIC_READDATA16_REG;
		QSPIC_WAIT_NOT_BUSY();
		uint32_t datum = QSPIC_RECVDATA_REG;
		data[0] = (uint8_t)datum;
		data[1] = (uint8_t)(datum >> 8);
		data += 2;
		data_len -= 2;
	}

	while (data_len) {
		QSPIC_READDATA16_REG;
		QSPIC_WAIT_NOT_BUSY();
		uint32_t datum = QSPIC_RECVDATA_REG;
		*data++ = (uint8_t)datum;
		data_len--;
	}
}

static void qspi_write_then_read(const qspi_xfer_desc_t *desc) {
	switch (desc->mode) {
	case QSPI_MODE_SIO:
		QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_SIO_EN;
		break;
	case QSPI_MODE_DIO:
		QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_DIO_EN;
		break;
	case QSPI_MODE_QIO:
		QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_QIO_EN;
		break;
	}

	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_ENABLE_BUS;
	if ((desc->tx_data && desc->tx_len) || desc->dummy_cycles_after_tx) {
		qspi_tx(desc);
	}
	if (desc->rx_data && desc->rx_len) {
		qspi_rx(desc);
	}
	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_DISABLE_BUS;
}

static void qspic_deassert_reassert_cs(void) {
	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_DISABLE_BUS;
	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_ENABLE_BUS;
}

int main(void) {
	P0_DIR_REG |= 0x03;
	P0_MODE_REG |= 0x01;

	UART_CTRL_REG = UART_CTRL_REG_UART_TEN | UART_CTRL_REG_UART_REN;

	uart_puts("Port 0 direction register should be at @0x");
	uart_putlong_hex((unsigned long)&P0_DIR_REG);
	uart_puts("\r\n");

//	P0_DIR_REG |= 0x0c;
//	P0_DATA_REG &= ~0x02;

	// QSPI clock config
	/* This could also be a clock source selector instead of a divider
	   Selecting DIV2 does result in half the frequency on QSPI clk though */
	CLK_PER10_DIV_REG &= ~CLK_PER10_DIV_REG_QSPI_DIV_MASK;
	CLK_PER10_DIV_REG |= CLK_PER10_DIV_REG_QSPI_DIV_1;
	CLK_AMBA_REG &= ~CLK_AMBA_REG_HCLK_DIV_MASK;
	CLK_AMBA_REG &= ~CLK_AMBA_REG_PCLK_DIV_MASK;
	CLK_AMBA_REG |= CLK_AMBA_REG_HCLK_DIV_2;
	/* SRAM enable does not seem to relate to QSPI, skipping it does not have any effect */
//	CLK_AMBA_REG |= CLK_AMBA_REG_SRAM1_EN;
	CLK_AMBA_REG |= CLK_AMBA_REG_PCLK_DIV_1;
	/* Not sure what this does, removing it does not seem to have any effect */
	CLK_PER10_DIV_REG |= CLK_PER10_DIV_REG_QSPI_EN;

	// QSPI enable
	QSPIC_DEASSERT_CS();
	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_SIO_EN;

	/* QSPI IO config, release reset, DO idle level high */
	/* 0x20 = RST/IO3 high */
	/* 0x10 = WP/IO2 high */
	/* 0x08 = RST/IO3 output enable */
	/* 0x04 = WP/IO2 output enable */
	/* 0x02 = DO/IO0 and CLK idle level high */
	/* 0x01 = output disable */
	QSPIC_CFG_REG = QSPIC_CFG_REG_IO3_RST_DATA | QSPIC_CFG_REG_IO3_RST_OEN | QSPIC_CFG_REG_IO2_WP_OEN;
	/* While set in the bootrom those three do not seem to be strictly required */
	QSPIC_UNKNOWN_REG1 |= (1 << 15);
	QSPIC_UNKNOWN_REG2 = 0x105;

	/* Even unsetting the bits from above explicitly does not seem to have any effect ... */
//	QSPIC_UNKNOWN_REG1 &= ~(1 << 15);
//	QSPIC_UNKNOWN_REG2 &= ~0x105;

	for (volatile unsigned int i = 0; i < 314; i++);
	for (volatile unsigned int i = 0; i < 480; i++) {
		if (QSPIC_UNKNOWN_REG3 & (1 << 3)) {
			break;
		}
	}

/*
	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_QIO_EN;
	QSPIC_WRITEDATA8_REG = 0xff;
	QSPIC_WAIT_NOT_BUSY();

	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_DIO_EN;
	QSPIC_WRITEDATA8_REG = 0xff;
	QSPIC_WAIT_NOT_BUSY();

	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_SIO_EN;
	qspic_deassert_reassert_cs();
	QSPIC_WRITEDATA8_REG = 0xf5;
	QSPIC_WAIT_NOT_BUSY();

	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_DISABLE_BUS;
//	qspic_deassert_reassert_cs();
//	QSPIC_WRITEDATA8_REG = 0xaa;
	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_QIO_EN;
	QSPIC_WRITEDATA16_REG = 0xaa55;
	QSPIC_WAIT_NOT_BUSY();

	qspic_deassert_reassert_cs();

	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_QIO_EN;
	QSPIC_WRITEDATA16_REG = 0xaa55;
	QSPIC_WAIT_NOT_BUSY();

	QSPIC_CTRL_REG16 = QSPIC_CTRL_REG_DISABLE_BUS;
*/
	uint8_t tx_data[256];
	for (int i = 0; i < 256; i++) {
		tx_data[i] = i;
	}
	uint8_t rx_data[256];
	qspi_xfer_desc_t desc = {
		.mode = QSPI_MODE_SIO,
		.tx_data = tx_data,
		.tx_len = sizeof(tx_data),
		.dummy_cycles_after_tx = 256,
		.rx_data = rx_data,
		.rx_len = sizeof(rx_data)
	};

	qspi_write_then_read(&desc);
/*
	unsigned int aux_clk_config = CLK_AUX2_REG;
	dump_reg16("CLK_AUX2_REG", CLK_AUX2_REG);
	CLK_AUX2_REG |= CLK_AUX2_REG_SW_QSPIC_EN;
	dump_reg16("CLK_AUX2_REG", CLK_AUX2_REG);
/*
	dump_reg16("QSPIC_BURSTCMDA_REG", QSPIC_BURSTCMDA_REG);
	QSPIC_BURSTCMDA_REG = 0xa5;
	dump_reg16("QSPIC_BURSTCMDA_REG", QSPIC_BURSTCMDA_REG);
	dump_reg16("QSPIC_BURSTCMDB_REG", QSPIC_BURSTCMDB_REG);
	QSPIC_BURSTCMDB_REG = 0xa5a5a5a5L;
	dump_reg16("QSPIC_BURSTCMDB_REG", QSPIC_BURSTCMDB_REG);
*/
	dump_reg16("P2_00_MODE_REG", MMIO16(0xFF48A0));

/*
	QSPIC_CTRLBUS_REG = 0x09;
	QSPIC_CTRLMODE_REG = 0x3c;
*/

//	QSPIC_WRITEDATA32_REG = 0xaaaaaaaa;

	uart_puts("Bootrom2:\r\n");
	uart_hexdump((void *)0xFEF000, 0x800);
	uart_puts("\r\nBootrom dump complete\r\n");
/*
	for (unsigned int port = 0; port <= 7; port++) {
		for (unsigned int pin = 0; pin < 8; pin++) {
			WATCHDOG_REG = 0xff;
			unsigned int found = 0;
			for (unsigned int idx = 0; idx < ARRAY_SIZE(pin_blacklist); idx++) {
				const port_pin_t *entry = &pin_blacklist[idx];
				if (entry->port == port && (entry->pin == -1 || entry->pin == pin)) {
					found = 1;
					break;
				}
			}
			if (found) {
				uart_puts("Skipping ");
				uart_putint(port);
				uart_puts(".");
				uart_putint(pin);
				uart_puts(", blacklisted\r\n");
			} else {
				toggle_pin(port, pin);
				for (volatile unsigned int i = 0; i < 10000; i++);
			}
		}
	}
*/
/*
	for (unsigned int i = 0; i <= 100; i++) {
		uart_putint(i);
		uart_puts("\r\n");
	}
*/
	for (volatile unsigned int i = 0; i < 10000; i++);
	reset();

	while (1) {
		WATCHDOG_REG = 0xff;
/*
	while (1) {
		UART_RX_TX_REG = 0x42;
		while (!(UART_CTRL_REG & UART_CTRL_REG_TI)) {
			if (UART_CTRL_REG & UART_CTRL_REG_RI) {
				UART_CLEAR_RX_INT_REG = 1;
			}
		}
		UART_CLEAR_TX_INT_REG = 1;
	}
*/
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
