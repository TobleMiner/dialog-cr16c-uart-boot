#include <stddef.h>
#include <stdint.h>

#include "clock.h"
#include "dma.h"
#include "gpio.h"
#include "irq.h"
#include "qspi.h"
#include "system.h"
#include "uart.h"
#include "util.h"
#include "watchdog.h"

#define JEDEC_CMD_RDID		0x9F
#define JEDEC_CMD_RDSFDP	0x5A
#define JEDEC_CMD_WREN		0x06
#define JEDEC_CMD_WRDI		0x04
#define JEDEC_CMD_RDSR		0x05
#define JEDEC_RDSR_WIP		(1 << 0)
#define JEDEC_RDSR_WEL		(1 << 1)

#define UART_CMD_PING		0x01
#define UART_CMD_CANGE_BAUDRATE	0x02
#define UART_CMD_FLASH_INFO	0x03
#define UART_CMD_ERASE_SECTOR	0x04
#define UART_CMD_PROGRAM_PAGE	0x05
#define UART_CND_RESET		0x06

typedef struct jedec_nor_flash_info {
	uint32_t size_bytes;
} jedec_nor_flash_info_t;

static void print_trap(const char *trap) {
	uart_puts("\r\n=======TRAP=========\r\n");
	uart_puts(trap);
	uart_puts("\r\n======ENDTRAP=======\r\n");
}

static void print_trap_reset(const char *trap) {
	print_trap(trap);
	system_reset();
}

static void svc_trap(void) {
	print_trap("Supervisor call");
}

static void dvz_trap(void) {
	print_trap_reset("Divide by zero");
}

static void flg_trap(void) {
	print_trap("Flag");
}

static void bpt_trap(void) {
	print_trap("Breakpoint");
	while (1);
}

static void trc_trap(void) {
	print_trap("Trace");
}

static void und_trap(void) {
	print_trap_reset("Undefined instruction");
}

static void iad_trap(void) {
	print_trap_reset("Illegal address");
}

static void dbg_trap(void) {
	print_trap("Debug");
}

static volatile unsigned int num_rx_interrupts = 0;
static void uart_rx_int(void) {
	UART_RX_TX_REG;
	UART_CLEAR_RX_INT_REG = 1;
	RESET_INT_PENDING_REG = RESET_INT_PENDING_REG_UART_RI_INT_PEND;
	num_rx_interrupts++;
}

vector_table_t vector_table = {
	.svc_trap = svc_trap,
	.dvz_trap = dvz_trap,
	.flg_trap = flg_trap,
	.bpt_trap = bpt_trap,
	.trc_trap = trc_trap,
	.und_trap = und_trap,
	.iad_trap = iad_trap,
	.dbg_trap = dbg_trap,
	.uart_ri_int = uart_rx_int,
};

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

static void qspic_deassert_reassert_cs(void) {
	QSPIC_CTRL_REG = QSPIC_CTRL_REG_DISABLE_BUS;
	QSPIC_CTRL_REG = QSPIC_CTRL_REG_ENABLE_BUS;
}

const uint8_t sfdp_read_header_cmd[] = { JEDEC_CMD_RDSFDP, 0x00, 0x00, 0x00 };

static void qspic_read_parameter_table(uint8_t *parameter_header, jedec_nor_flash_info_t *flash_info) {
	uint8_t parameter_table[64];
	unsigned int table_len = (unsigned int)parameter_header[3] * 4;
	if (table_len > sizeof(parameter_table)) {
		uart_puts("Parameter table > 64 byte, skipping");
		return ;
	}
	unsigned int table_id = parameter_header[0];
	uint8_t read_parameter_table_cmd[] = { JEDEC_CMD_RDSFDP, parameter_header[6], parameter_header[5], parameter_header[4] };
	qspi_xfer_desc_t read_param_table_desc = {
		.mode = QSPI_MODE_SIO,
		.tx_data = read_parameter_table_cmd,
		.tx_len = sizeof(read_parameter_table_cmd),
		.dummy_cycles_after_tx = 1,
		.rx_data = parameter_table,
		.rx_len = table_len
	};
	qspi_write_then_read(&read_param_table_desc);
	unsigned int table_type = parameter_header[0];
	uart_puts("Parameter table @0x");
	uart_putbyte_hex(parameter_header[6]);
	uart_putbyte_hex(parameter_header[5]);
	uart_putbyte_hex(parameter_header[4]);
	uart_puts(": ");
	uart_hexdump(parameter_table, table_len);
	uart_puts("\r\n");

	if (table_type == 0x00) {
		uint32_t raw_size = (uint32_t)parameter_table[4] |
				    (uint32_t)parameter_table[5] << 8 |
				    (uint32_t)parameter_table[6] << 16 |
				    (uint32_t)parameter_table[7] << 24;

		uart_puts("Raw flash size 0x");
		uart_putlong_hex(raw_size);
		uart_puts("\r\n");
		if (raw_size & (1L << 31)) {
			flash_info->size_bytes = (1 << raw_size) / 8UL;
		} else {
			flash_info->size_bytes = (raw_size + 1) / 8UL;
		}
	}
}

static void qspic_read_sfdp(jedec_nor_flash_info_t *flash_info) {
	uint8_t sfdp_header[8];
	qspi_xfer_desc_t desc = {
		.mode = QSPI_MODE_SIO,
		.tx_data = sfdp_read_header_cmd,
		.tx_len = sizeof(sfdp_read_header_cmd),
		.dummy_cycles_after_tx = 1,
		.rx_data = sfdp_header,
		.rx_len = sizeof(sfdp_header)
	};
	qspi_write_then_read(&desc);
	uart_puts("SFDP header: ");
	uart_hexdump(sfdp_header, sizeof(sfdp_header));
	uart_puts("\r\n");

	unsigned int num_parameter_header = sfdp_header[6] + 1;
	uart_puts("Found ");
	uart_putint(num_parameter_header);
	uart_puts(" parameter headers\r\n");

	for (unsigned int hdr_idx = 0; hdr_idx < num_parameter_header; hdr_idx++) {
		uint8_t parameter_header[8];

		unsigned int address = sizeof(sfdp_header) + sizeof(parameter_header) * hdr_idx;
		uint8_t read_parameter_header_cmd[] = { JEDEC_CMD_RDSFDP, (address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff };
		qspi_xfer_desc_t read_param_header_desc = {
			.mode = QSPI_MODE_SIO,
			.tx_data = read_parameter_header_cmd,
			.tx_len = sizeof(read_parameter_header_cmd),
			.dummy_cycles_after_tx = 1,
			.rx_data = parameter_header,
			.rx_len = sizeof(parameter_header)
		};
		qspi_write_then_read(&read_param_header_desc);
		uart_puts("Parameter header ");
		uart_putint(hdr_idx);
		uart_puts(": ");
		uart_hexdump(parameter_header, sizeof(parameter_header));
		uart_puts("\r\n");
		qspic_read_parameter_table(parameter_header, flash_info);
	}
}

#define FUNCTION_ADDRESS(funcptr_) (((intptr_t)funcptr_) << 1)

extern void real_vector_table;

int main(void) {
	uart_init();
	uart_puts("Port 0 direction register should be at @0x");
	uart_putlong_hex((unsigned long)&P0_DIR_REG);
	uart_puts("\r\n");

	uart_puts("SVC trap handler: 0x");
	uart_putlong_hex(FUNCTION_ADDRESS(svc_trap));
	uart_puts("\r\n");
	uart_puts("main(): 0x");
	uart_putlong_hex(FUNCTION_ADDRESS(main));
	uart_puts("\r\n");

	funcp_t *vectors = &vector_table;
	for (int i = 0; i < sizeof(vector_table) / sizeof(funcp_t); i++) {
		uart_puts("Vector ");
		uart_putint(i);
		uart_puts(" 0x");
		uart_putlong_hex((unsigned long)vectors[i]);
		uart_puts("\r\n");
	}


//	P0_DIR_REG |= 0x0c;
//	P0_DATA_REG &= ~0x02;

	qspi_init();

/*
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
*/
	watchdog_reset();

	asm("excp svc");
	uart_puts("SVC call returned\r\n");

	jedec_nor_flash_info_t flash_info = { 0 };
	qspic_read_sfdp(&flash_info);
	uart_puts("Flash size ");
	uart_putlong(flash_info.size_bytes);
	uart_puts(" bytes\r\n");
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

/*
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
	for (unsigned int i = 0; i < 20; i++) {
//		SET_INT_PENDING_REG = RESET_INT_PENDING_REG_UART_RI_INT_PEND;
		unsigned int num_irqs;
		asm("di");
		num_irqs = num_rx_interrupts;
		asm("ei");

		uart_puts("Got ");
		uart_putint(num_irqs);
		uart_puts(" RX irqs\r\n");
		watchdog_reset();
		for (volatile unsigned int j = 0; j < 10000; j++);
	}
	system_reset();

	while (1) {
		watchdog_reset();
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
