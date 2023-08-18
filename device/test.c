#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "chipid.h"
#include "clock.h"
#include "crc32.h"
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

#define HEADER_BYTE		0xA5

#define UART_CMD_PING		0x00
#define UART_CMD_SET_BAUDRATE	0x01
#define UART_CMD_FLASH_INFO	0x02
#define UART_CMD_ERASE_SECTOR	0x03
#define UART_CMD_PROGRAM_PAGE	0x04
#define UART_CMD_RESET		0x05
#define UART_CMD_READ_FLASH	0x06
#define UART_CMD_CHECKSUM	0x07
#define UART_CMD_CHIPID		0x08

#define RESPONSE_INVALID_CRC	0x00
#define RESPONSE_CMD_OK		0x01
#define RESPONSE_CMD_INVALID	0x02
#define RESPONSE_PARAM_SHORT	0x03
#define RESPONSE_OK		0x04
#define RESPONSE_DEBUG		0x05
#define RESPONSE_INVALID_PARAM	0x06
#define RESPONSE_ONLINE		0x07
#define RESPONSE_FLASH_TIMEOUT	0x08
#define RESPONSE_CHECKSUM	0x09
#define RESPONSE_FLASH_INFO	0x0A
#define RESPONSE_CHIPID		0x0B

#define TIMEOUT_HEADER		0x100000
#define TIMEOUT_CMD		0x1000
#define TIMEOUT_PARAM		0x10000
#define TIMEOUT_SECTOR_ERASE	0x4000
#define TIMEOUT_PAGE_PROGRAM	0x400

typedef enum cmd_state {
	CMD_STATE_WAIT_HEADER,
	CMD_STATE_WAIT_CMD,
	CMD_STATE_WAIT_PARAM
} cmd_state_t;

struct cmd_handler;
typedef struct cmd_handler cmd_handler_t;

struct cmd_handler {
	void (*call)(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len);
	unsigned int min_param_len;
};

typedef struct jedec_nor_flash_sector {
	uint8_t erase_opcode;
	uint8_t size_exponent;
} jedec_nor_flash_sector_t;

typedef struct jedec_nor_flash_info {
	uint32_t size_bytes;
	uint8_t erase_opcode_4kib;
	jedec_nor_flash_sector_t erase_sector_types[4];
} jedec_nor_flash_info_t;

static jedec_nor_flash_info_t flash_info_g = { 0 };

static uint32_t read_le32(const void *data) {
	const uint8_t *data8 = data;
	return	(uint32_t)data8[0] |
		(uint32_t)data8[1] << 8 |
		(uint32_t)data8[2] << 16 |
		(uint32_t)data8[3] << 24;
}

static void write_le32(void *data, uint32_t val) {
	uint8_t *data8 = data;
	data8[0] = (val >> 0) & 0xff;
	data8[1] = (val >> 8) & 0xff;
	data8[2] = (val >> 16) & 0xff;
	data8[3] = (val >> 24) & 0xff;
}

static void send_response_with_payload_(uint8_t response, uint32_t id, uint32_t len) {
	uint8_t hdr[14];
	hdr[0] = HEADER_BYTE;
	hdr[1] = response;
	write_le32(&hdr[2], id);
	write_le32(&hdr[6], len);
	uint32_t crc = crc32_init();
	crc = crc32_update(crc, hdr, 10);
	crc = crc32_final(crc);
	write_le32(&hdr[10], crc);
	uart_write(hdr, sizeof(hdr));
}

static void send_response_with_payload(uint8_t response, uint32_t id, const void *data, uint32_t len) {
	send_response_with_payload_(response, id, len);
	uint8_t crc_buf[4];
	uint32_t crc = crc32_init();
	crc = crc32_update(crc, data, len);
	crc = crc32_final(crc);
	write_le32(crc_buf, crc);
	uart_write(data, len);
	uart_write(crc_buf, sizeof(crc_buf));
}

static void debug_puts(const char *ptr) {
	send_response_with_payload(RESPONSE_DEBUG, 0xFFFFFFFF, ptr, strlen(ptr));
}

static void debug_putint(unsigned int val) {
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
	debug_puts(ptr);
}

static void debug_putlong(unsigned long val) {
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
	debug_puts(ptr);
}

#define NIBBLE_TO_HEX_CHAR(i) ((i) < 10 ? '0' + (i) : 'A' + ((i) - 10))

static void debug_putbyte_hex(unsigned char byt) {
	char str[3];
	str[0] = NIBBLE_TO_HEX_CHAR((byt >> 4) & 0xf);
	str[1] = NIBBLE_TO_HEX_CHAR(byt & 0xf);
	str[2] = 0;
	debug_puts(str);
}

static void send_response(uint8_t response, uint32_t id) {
	send_response_with_payload_(response, id, 0);
/*
	debug_puts("Short response to ");
	debug_putlong(id);
	debug_puts(" sent, code 0x");
	debug_putbyte_hex(response);
	debug_puts("\r\n");
*/
}

static void debug_putint_hex(unsigned int i) {
	debug_putbyte_hex(i >> 8);
	debug_putbyte_hex(i & 0xff);
}

static void debug_putlong_hex(unsigned long i) {
	debug_putint_hex(i >> 16);
	debug_putint_hex(i & 0xffff);
}

static void debug_hexdump(const void *ptr, unsigned int len) {
	send_response_with_payload_(RESPONSE_DEBUG, 0xFFFFFFFF, len * 2);
	const uint8_t *ptr8 = ptr;
	uint32_t crc = crc32_init();
	while (len--) {
		uint8_t byt = *ptr8++;
		char hexdata[2] = {
			NIBBLE_TO_HEX_CHAR((byt >> 4) & 0xf),
			NIBBLE_TO_HEX_CHAR((byt >> 0) & 0xf),
		};
		crc = crc32_update(crc, hexdata, sizeof(hexdata));
		uart_write(hexdata, sizeof(hexdata));
		watchdog_reset();

	}
	crc = crc32_final(crc);
	uint8_t crc_buf[4];
	write_le32(crc_buf, crc);
	uart_write(crc_buf, sizeof(crc_buf));
}

static void print_trap(const char *trap) {
	debug_puts("\r\n=======TRAP=========\r\n");
	debug_puts(trap);
	debug_puts("\r\n======ENDTRAP=======\r\n");
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

static uint8_t uart_rx_buf[1024];
static unsigned int uart_rx_dma_ptr = 0;
static unsigned int uart_rx_read_ptr = 0;

static unsigned int uart_rx_buffered_data(void) {
	unsigned int data_in_transit = DMAX_IDX_REG(DMA_UART_RX);
	unsigned int effective_rx_dma_ptr = uart_rx_dma_ptr + data_in_transit;
	if (effective_rx_dma_ptr >= uart_rx_read_ptr) {
		return effective_rx_dma_ptr - uart_rx_read_ptr;
	} else {
		return sizeof(uart_rx_buf) - uart_rx_read_ptr + effective_rx_dma_ptr;
	}
}

static bool uart_data_available(void) {
	unsigned int data_in_transit = DMAX_IDX_REG(DMA_UART_RX);
	return (uart_rx_dma_ptr + data_in_transit) != uart_rx_read_ptr;
}

static uint8_t uart_read_byte(void) {
	uint8_t datum = uart_rx_buf[uart_rx_read_ptr++];
	uart_rx_read_ptr %= sizeof(uart_rx_buf);
	return datum;
}

static uint32_t uart_read_u32_le(void) {
	return	(uint32_t)uart_read_byte() |
		(uint32_t)uart_read_byte() << 8 |
		(uint32_t)uart_read_byte() << 16 |
		(uint32_t)uart_read_byte() << 24;
}

static void uart_read(void *ptr, unsigned int len) {
	uint8_t *ptr8 = ptr;
	while (len--) {
		*ptr8++ = uart_read_byte();
	}
}

static void *uart_get_read_ptr(void) {
	return &uart_rx_buf[uart_rx_read_ptr];
}

static void uart_advance_read_ptr(unsigned int increment) {
	uart_rx_read_ptr += increment;
}

static uint8_t uart_tx_buf[1024];
static unsigned int uart_tx_dma_ptr = 0;
static unsigned int uart_tx_write_ptr = 0;

static volatile unsigned int num_rx_interrupts = 0;
bool uart_rx_irq_flag = false;
static void uart_rx_int(void) {
	/*
	UART_RX_TX_REG;
	UART_CLEAR_RX_INT_REG = 1;
	RESET_INT_PENDING_REG = RESET_INT_PENDING_REG_UART_RI_INT_PEND;
	*/
	num_rx_interrupts++;
	uart_rx_irq_flag = true;
}

static unsigned int uart_tx_buffered_data(void) {
	if (uart_tx_write_ptr > uart_tx_dma_ptr) {
		return uart_tx_write_ptr - uart_tx_dma_ptr;
	} else {
		return sizeof(uart_tx_buf) - uart_tx_dma_ptr + uart_tx_write_ptr;
	}
}

bool tx_irq_flag = false;
bool tx_dma_active = false;
static void uart_start_tx_dma(void) {
	unsigned int num_bytes = uart_tx_buffered_data();
	DMAX_A_STARTL_REG(DMA_UART_TX) = (uint16_t)(uintptr_t)&uart_tx_buf[uart_tx_dma_ptr];
	DMAX_A_STARTH_REG(DMA_UART_TX) = (uint16_t)((uint32_t)&uart_tx_buf[uart_tx_dma_ptr] >> 16);
	DMAX_INT_REG(DMA_UART_TX) = num_bytes;
	DMAX_LEN_REG(DMA_UART_TX) = num_bytes;
	DMAX_CTRL_REG(DMA_UART_TX) |= DMAX_CTRL_REG_DMA_ON;
	tx_dma_active = true;
}

static void uart_tx_int(void) {
	unsigned int xfer_len = DMAX_LEN_REG(DMA_UART_TX);
	uart_tx_dma_ptr += xfer_len;
	uart_tx_dma_ptr %= sizeof(uart_tx_buf);

	if (uart_tx_dma_ptr == uart_tx_write_ptr) {
		DMAX_CTRL_REG(DMA_UART_TX) &= ~DMAX_CTRL_REG_DMA_ON;
		tx_dma_active = false;
	} else {
		uart_start_tx_dma();
	}
	tx_irq_flag = true;
}

static unsigned int uart_tx_buffer_space(void) {
	return sizeof(uart_tx_buf) - uart_tx_buffered_data();
}

static unsigned int uart_tx_buffer_copy(const void *src, unsigned int len) {
	const uint8_t *src8 = src;
	unsigned int bytes_to_end_of_buffer = sizeof(uart_tx_buf) - uart_tx_write_ptr;
	unsigned int bytes_to_copy = len;
	unsigned int new_tx_write_ptr = uart_tx_write_ptr;
	if (bytes_to_copy > bytes_to_end_of_buffer) {
		bytes_to_copy = bytes_to_end_of_buffer;
	}
	memcpy(&uart_tx_buf[new_tx_write_ptr], src8, bytes_to_copy);
	new_tx_write_ptr += bytes_to_copy;
	new_tx_write_ptr %= sizeof(uart_tx_buf);
	src8 += bytes_to_copy;
	if (bytes_to_copy < len) {
		memcpy(&uart_tx_buf[new_tx_write_ptr], src8, len - bytes_to_copy);
		new_tx_write_ptr += len - bytes_to_copy;
		new_tx_write_ptr %= sizeof(uart_tx_buf);
	}

	return new_tx_write_ptr;
}

/*
static void uart_write(const void *ptr, unsigned int len) {
	const uint8_t *ptr8 = ptr;
	tx_irq_flag = false;
	while (len) {
		disable_interrupts();
		unsigned int buffer_space = uart_tx_buffer_space();
		enable_interrupts();
		unsigned int bytes_to_copy = buffer_space;
		if (len < bytes_to_copy) {
			bytes_to_copy = len;
		}
		unsigned int tx_write_ptr = uart_tx_buffer_copy(ptr, bytes_to_copy);
		disable_interrupts();
		uart_tx_write_ptr = tx_write_ptr;
		if (!tx_dma_active) {
			uart_start_tx_dma();
		}
		enable_interrupts();
		while (!tx_irq_flag);
		tx_irq_flag = false;
	}
}

static void uart_write_string(const char *str) {
	uart_write(str, strlen(str));
}

static void uart_flush(void) {
	disable_interrupts();
	unsigned int buffered_bytes = uart_tx_buffered_data();
	enable_interrupts();
	while (buffered_bytes) {
		while (!tx_irq_flag) {
			watchdog_reset();
		}
		tx_irq_flag = false;

		disable_interrupts();
		buffered_bytes = uart_tx_buffered_data();
		enable_interrupts();
	}
}

static void uart_read(void *ptr, unsigned int len) {
	uart_rx_irq_flag = false;
	DMAX_A_STARTL_REG(DMA_UART_RX) = (uint16_t)(uintptr_t)&UART_RX_TX_REG;
	DMAX_A_STARTH_REG(DMA_UART_RX) = (uint16_t)((uint32_t)&UART_RX_TX_REG >> 16);
	DMAX_B_STARTL_REG(DMA_UART_RX) = (uint16_t)(uintptr_t)ptr;
	DMAX_B_STARTH_REG(DMA_UART_RX) = (uint16_t)((uint32_t)ptr >> 16);
	DMAX_INT_REG(DMA_UART_RX) = sizeof(len);
	DMAX_LEN_REG(DMA_UART_RX) = sizeof(len);
	DMAX_CTRL_REG(DMA_UART_RX) =
		DMAX_CTRL_REG_DMA_PRIO_MIDHIGH |
		DMAX_CTRL_REG_BINC |
		DMAX_CTRL_REG_DREQ_MODE |
		DMAX_CTRL_REG_DINT_MODE |
		DMAX_CTRL_REG_BW_BYTE;
	DMAX_CTRL_REG(DMA_UART_RX) |= DMAX_CTRL_REG_DMA_ON;
	while (!uart_rx_irq_flag);
}
*/

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
	.uart_ti_int = uart_tx_int,
};

static void qspic_read_erase_sector_size(uint8_t sector_desc[2], jedec_nor_flash_sector_t *sector_info) {
	sector_info->size_exponent = sector_desc[0];
	sector_info->erase_opcode = sector_desc[1];
	if (sector_info->size_exponent != 0xff) {
		debug_puts("Sector size 2^");
		debug_putint(sector_info->size_exponent);
		debug_puts(" erase opcode is 0x");
		debug_putbyte_hex(sector_info->erase_opcode);
		debug_puts("\r\n");
	}
}

static void qspic_read_parameter_table_0(uint8_t parameter_table[64], jedec_nor_flash_info_t *flash_info) {
	flash_info->erase_opcode_4kib = parameter_table[2];
	debug_puts("4KiB erase opcode 0x");
	debug_putbyte_hex(flash_info->erase_opcode_4kib);
	debug_puts("\r\n");

	uint32_t raw_size = (uint32_t)parameter_table[4] |
			    (uint32_t)parameter_table[5] << 8 |
			    (uint32_t)parameter_table[6] << 16 |
			    (uint32_t)parameter_table[7] << 24;

	debug_puts("Raw flash size 0x");
	debug_putlong_hex(raw_size);
	debug_puts("\r\n");
	if (raw_size & (1L << 31)) {
		flash_info->size_bytes = (1 << raw_size) / 8UL;
	} else {
		flash_info->size_bytes = (raw_size + 1) / 8UL;
	}

	for (int i = 0; i < 4; i++) {
		qspic_read_erase_sector_size(&parameter_table[7 * 4 + i / 2], &flash_info->erase_sector_types[i]);
	}
}

static const uint8_t sfdp_read_header_cmd[] = { JEDEC_CMD_RDSFDP, 0x00, 0x00, 0x00 };
static void qspic_read_parameter_table(uint8_t *parameter_header, jedec_nor_flash_info_t *flash_info) {
	uint8_t parameter_table[64];
	unsigned int table_len = (unsigned int)parameter_header[3] * 4;
	if (table_len > sizeof(parameter_table)) {
		debug_puts("Parameter table > 64 byte, skipping");
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
	debug_puts("Parameter table @0x");
	debug_putbyte_hex(parameter_header[6]);
	debug_putbyte_hex(parameter_header[5]);
	debug_putbyte_hex(parameter_header[4]);
	debug_puts(": ");
	debug_hexdump(parameter_table, table_len);
	debug_puts("\r\n");

	if (table_type == 0x00) {
		qspic_read_parameter_table_0(parameter_table, flash_info);
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
	debug_puts("SFDP header: ");
	debug_hexdump(sfdp_header, sizeof(sfdp_header));
	debug_puts("\r\n");

	unsigned int num_parameter_header = sfdp_header[6] + 1;
	debug_puts("Found ");
	debug_putint(num_parameter_header);
	debug_puts(" parameter headers\r\n");

	if (num_parameter_header == 0x100) {
		debug_puts("Flash does not seem to support SFDP, skipping auto detection\r\n");
		return;
	}

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
		debug_puts("Parameter header ");
		debug_putint(hdr_idx);
		debug_puts(": ");
		debug_hexdump(parameter_header, sizeof(parameter_header));
		debug_puts("\r\n");
		qspic_read_parameter_table(parameter_header, flash_info);
	}
}

#define FUNCTION_ADDRESS(funcptr_) (((intptr_t)funcptr_) << 1)

static void start_uart_rx_dma(void *ptr, unsigned int len) {
	DMAX_A_STARTL_REG(DMA_UART_RX) = (uint16_t)(uintptr_t)&UART_RX_TX_REG;
	DMAX_A_STARTH_REG(DMA_UART_RX) = (uint16_t)((uint32_t)&UART_RX_TX_REG >> 16);
	DMAX_B_STARTL_REG(DMA_UART_RX) = (uint16_t)(uintptr_t)ptr;
	DMAX_B_STARTH_REG(DMA_UART_RX) = (uint16_t)((uint32_t)ptr >> 16);
	DMAX_INT_REG(DMA_UART_RX) = len;
	DMAX_LEN_REG(DMA_UART_RX) = len;
	DMAX_CTRL_REG(DMA_UART_RX) =
		DMAX_CTRL_REG_DMA_PRIO_MIDHIGH |
		DMAX_CTRL_REG_CIRCULAR |
		DMAX_CTRL_REG_BINC |
		DMAX_CTRL_REG_DREQ_MODE |
		DMAX_CTRL_REG_DINT_MODE |
		DMAX_CTRL_REG_BW_BYTE;
	DMAX_CTRL_REG(DMA_UART_RX) |= DMAX_CTRL_REG_DMA_ON;
}

static void reset_uart_rx_dma(void) {
	start_uart_rx_dma(uart_rx_buf, sizeof(uart_rx_buf));
	uart_rx_read_ptr = 0;
}

static uint8_t read_flash_status_register(void) {
	const uint8_t read_status_register_cmd[] = { 0x05 };
	uint8_t status_register[1];
	qspi_xfer_desc_t read_status_register_desc = {
		.mode = QSPI_MODE_SIO,
		.tx_data = read_status_register_cmd,
		.tx_len = sizeof(read_status_register_cmd),
		.dummy_cycles_after_tx = 0,
		.rx_data = status_register,
		.rx_len = sizeof(status_register)
	};
	qspi_write_then_read(&read_status_register_desc);

	return status_register[0];
}

static bool wait_flash_write_finished(unsigned int timeout) {
	do {
		uint8_t status = read_flash_status_register();
		if (!(status & JEDEC_RDSR_WIP)) {
			return true;
		}
	} while (timeout--);

	return false;
}

static void call_ping_handler(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len) {
	send_response(RESPONSE_OK, id);
}

static void call_set_baudrate_handler(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len) {
	uint32_t baudrate = read_le32(param_data);
	if (uart_is_baudrate_attainable(baudrate)) {
		send_response(RESPONSE_OK, id);
		uart_flush();
		uart_set_baudrate(baudrate);

	} else {
		send_response(RESPONSE_INVALID_PARAM, id);
	}
}

static void call_flash_info_handler(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len) {
	uint8_t flash_info_buf[4];
	write_le32(flash_info_buf, flash_info_g.size_bytes);
	send_response_with_payload(RESPONSE_FLASH_INFO, id, flash_info_buf, sizeof(flash_info_buf));
}

static void flash_write_enable(void) {
	qspi_set_write_protect(false);

	const uint8_t write_enable_cmd[] = { 0x06 };
	const qspi_xfer_desc_t write_enable_desc = {
		.mode = QSPI_MODE_SIO,
		.tx_data = write_enable_cmd,
		.tx_len = sizeof(write_enable_cmd),
		.dummy_cycles_after_tx = 0,
		.rx_data = NULL,
		.rx_len = 0
	};
	qspi_write_then_read(&write_enable_desc);
}

static void call_erase_sector_handler(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len) {
	uint32_t address = read_le32(param_data);

	flash_write_enable();

	uint8_t erase_sector_cmd[] = { 0x20, (address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff };
	qspi_xfer_desc_t erase_sector_desc = {
		.mode = QSPI_MODE_SIO,
		.tx_data = erase_sector_cmd,
		.tx_len = sizeof(erase_sector_cmd),
		.dummy_cycles_after_tx = 0,
		.rx_data = NULL,
		.rx_len = 0
	};
	qspi_write_then_read(&erase_sector_desc);

	bool success = wait_flash_write_finished(TIMEOUT_SECTOR_ERASE);

	qspi_set_write_protect(true);

	if (success) {
		send_response(RESPONSE_OK, id);
	} else {
		send_response(RESPONSE_FLASH_TIMEOUT, id);
	}
}

static void call_program_page_handler(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len) {
	const uint8_t *param8 = param_data;
	uint32_t address = read_le32(&param8[0]);

	flash_write_enable();

	uint8_t program_sector_cmd[] = { 0x02, (address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff };
	qpsi_xfer_action_t actions[] = {
		{
			.action = QSPI_WRITE,
			.tx_data = program_sector_cmd,
			.len = sizeof(program_sector_cmd)
		},
		{
			.action = QSPI_WRITE,
			.tx_data = &param8[4],
			.len = 256
		}
	};

	qspi_scatter_transfer(QSPI_MODE_SIO, actions, ARRAY_SIZE(actions));

	bool success = wait_flash_write_finished(TIMEOUT_PAGE_PROGRAM);

	qspi_set_write_protect(true);

	if (success) {
		send_response(RESPONSE_OK, id);
	} else {
		send_response(RESPONSE_FLASH_TIMEOUT, id);
	}
}

static void call_reset_handler(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len) {
	send_response(RESPONSE_OK, id);
	system_reset();
}

static uint8_t flash_read_buffer[256];
static void call_read_flash_handler(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len) {
	const uint8_t *param8 = param_data;
	uint32_t start_address = read_le32(&param8[0]);
	uint32_t length = read_le32(&param8[4]);

/*
	debug_puts("Reading ");
	debug_putlong(length);
	debug_puts(" bytes from flash at 0x");
	debug_putlong_hex(start_address);
	debug_puts("\r\n");
*/
	send_response_with_payload_(RESPONSE_OK, id, length);

	uint32_t crc = crc32_init();
	while (length) {
		uint32_t read_length = length;
		if (read_length > sizeof(flash_read_buffer)) {
			read_length = sizeof(flash_read_buffer);
		}

		uint8_t read_flash_cmd[] = { 0x03, (start_address >> 16) & 0xff, (start_address >> 8) & 0xff, start_address & 0xff };
		qspi_xfer_desc_t desc = {
			.mode = QSPI_MODE_SIO,
			.tx_data = read_flash_cmd,
			.tx_len = sizeof(read_flash_cmd),
			.dummy_cycles_after_tx = 0,
			.rx_data = flash_read_buffer,
			.rx_len = read_length
		};
		qspi_write_then_read(&desc);
		crc = crc32_update(crc, flash_read_buffer, read_length);
		uart_write(flash_read_buffer, read_length);

		length -= read_length;
		start_address += read_length;
	}

	crc = crc32_final(crc);
	uint8_t crc_buf[4];
	write_le32(crc_buf, crc);
	uart_write(crc_buf, sizeof(crc_buf));

/*
	debug_puts("Read done, CRC embedded 0x");
	debug_putlong_hex(crc);
	debug_puts("\r\n");
*/
}

static void call_checksum_handler(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len) {
	const uint8_t *param8 = param_data;
	uint32_t start_address = read_le32(&param8[0]);
	uint32_t length = read_le32(&param8[4]);

	uint32_t crc = crc32_init();
	while (length) {
		uint32_t read_length = length;
		if (read_length > sizeof(flash_read_buffer)) {
			read_length = sizeof(flash_read_buffer);
		}

		uint8_t read_flash_cmd[] = { 0x03, (start_address >> 16) & 0xff, (start_address >> 8) & 0xff, start_address & 0xff };
		qspi_xfer_desc_t desc = {
			.mode = QSPI_MODE_SIO,
			.tx_data = read_flash_cmd,
			.tx_len = sizeof(read_flash_cmd),
			.dummy_cycles_after_tx = 0,
			.rx_data = flash_read_buffer,
			.rx_len = read_length
		};
		qspi_write_then_read(&desc);
		crc = crc32_update(crc, flash_read_buffer, read_length);
		watchdog_reset();

		length -= read_length;
		start_address += read_length;
	}

	/* Inner CRC of flash data */
	crc = crc32_final(crc);
	uint8_t crc_buf[4];
	write_le32(crc_buf, crc);
	uart_write(crc_buf, sizeof(crc_buf));

	send_response_with_payload(RESPONSE_CHECKSUM, id, crc_buf, sizeof(crc_buf));
}

static void call_chipid_handler(const cmd_handler_t *handler, uint32_t id, const void *param_data, unsigned int param_len) {
	chipid_t chipid;
	chipid_read(&chipid);
	uint8_t chipid_buf[] = {
		chipid.id1,
		chipid.id2,
		chipid.id3,
		chipid.mem_size,
		chipid.revision
	};
	send_response_with_payload(RESPONSE_CHIPID, id, chipid_buf, sizeof(chipid_buf));
}

static const cmd_handler_t cmd_handlers[] = {
	[UART_CMD_PING] = {
		.call = call_ping_handler,
		.min_param_len = 0,
	},
	[UART_CMD_SET_BAUDRATE] = {
		.call = call_set_baudrate_handler,
		.min_param_len = 4,
	},
	[UART_CMD_FLASH_INFO] = {
		.call = call_flash_info_handler,
		.min_param_len = 0,
	},
	[UART_CMD_ERASE_SECTOR] = {
		.call = call_erase_sector_handler,
		.min_param_len = 4,
	},
	[UART_CMD_PROGRAM_PAGE] = {
		.call = call_program_page_handler,
		.min_param_len = 4 + 256,
	},
	[UART_CMD_RESET] = {
		.call = call_reset_handler,
		.min_param_len = 0,
	},
	[UART_CMD_READ_FLASH] = {
		.call = call_read_flash_handler,
		.min_param_len = 8,
	},
	[UART_CMD_CHECKSUM] = {
		.call = call_checksum_handler,
		.min_param_len = 8,
	},
	[UART_CMD_CHIPID] = {
		.call = call_chipid_handler,
		.min_param_len = 0,
	},
};

static void dispatch_cmd(const cmd_handler_t *handler, uint32_t id, const void *parameter_data, uint32_t parameter_len) {
	handler->call(handler, id, parameter_data, parameter_len);
}

int main(void) {
	uart_init();

	DMAX_B_STARTL_REG(DMA_UART_TX) = (uint16_t)(uintptr_t)&UART_RX_TX_REG;
	DMAX_B_STARTH_REG(DMA_UART_TX) = (uint16_t)((uint32_t)&UART_RX_TX_REG >> 16);
	DMAX_CTRL_REG(DMA_UART_TX) =
		DMAX_CTRL_REG_DMA_PRIO_LOW |
		DMAX_CTRL_REG_AINC |
		DMAX_CTRL_REG_DREQ_MODE |
		DMAX_CTRL_REG_DINT_MODE |
		DMAX_CTRL_REG_BW_BYTE;

	reset_uart_rx_dma();

    // empty debug to avoid first actual debug out being consumed by python frontend lol
    debug_puts("");

/*
	uint8_t data[] = { 0xff, 0x42 };
	uint32_t crc32 = crc32_init();
	crc32 = crc32_update(crc32, data, sizeof(data));
	crc32 = crc32_final(crc32);
	debug_puts("CRC32 (embedded): 0x");
	debug_putlong_hex(crc32);
	debug_puts("\r\n");

	debug_puts("Port 0 direction register should be at @0x");
	debug_putlong_hex((unsigned long)&P0_DIR_REG);
	debug_puts("\r\n");

	debug_puts("SVC trap handler: 0x");
	debug_putlong_hex(FUNCTION_ADDRESS(svc_trap));
	debug_puts("\r\n");
	debug_puts("main(): 0x");
	debug_putlong_hex(FUNCTION_ADDRESS(main));
	debug_puts("\r\n");

	funcp_t *vectors = &vector_table;
	for (int i = 0; i < sizeof(vector_table) / sizeof(funcp_t); i++) {
		debug_puts("Vector ");
		debug_putint(i);
		debug_puts(" 0x");
		debug_putlong_hex((unsigned long)vectors[i]);
		debug_puts("\r\n");
	}
*/

//	P0_DIR_REG |= 0x0c;
//	P0_DATA_REG &= ~0x02;

    chipid_t chip_id;
    chipid_read(&chip_id);
    bool has_qspi = true;
    if (chip_id.id1 == '4' && chip_id.id2 == '8' && chip_id.id3 == '1') {
        has_qspi = false;
    }

    if (has_qspi) {
        qspi_init();
    }

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
	debug_puts("SVC call returned\r\n");

    if (has_qspi) {
        qspic_read_sfdp(&flash_info_g);
        debug_puts("Flash size ");
        debug_putlong(flash_info_g.size_bytes);
        debug_puts(" bytes\r\n");
    }
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
/*
	for (volatile unsigned int j = 0; j < 10000; j++);
	const char *dma_test_string = "Hello World, this data was transferred via DMA\r\n";

	UART_CLEAR_TX_INT_REG = 1;
	DMAX_A_STARTL_REG(DMA_UART_TX) = (uint16_t)(uintptr_t)dma_test_string;
	DMAX_A_STARTH_REG(DMA_UART_TX) = (uint16_t)((uint32_t)dma_test_string >> 16);
	DMAX_INT_REG(DMA_UART_TX) = strlen(dma_test_string) * 2;
	DMAX_LEN_REG(DMA_UART_TX) = strlen(dma_test_string);
	DMAX_CTRL_REG(DMA_UART_TX) |= DMAX_CTRL_REG_DMA_ON;
*/
	for (int i = 0; i < 3; i++) {
		send_response(RESPONSE_ONLINE, 0xFFFFFFFF);
	}

	cmd_state_t cmd_state = CMD_STATE_WAIT_HEADER;
	uint32_t parameter_len;
	uint32_t id;
	const cmd_handler_t *current_handler;
	unsigned long timeout = TIMEOUT_HEADER;
	while (1) {
		if (cmd_state == CMD_STATE_WAIT_HEADER) {
			if (uart_data_available()) {
				uint8_t datum = uart_read_byte();
				if (datum == HEADER_BYTE) {
					cmd_state = CMD_STATE_WAIT_CMD;
					timeout = TIMEOUT_CMD;
				}
			}
		}

		if (cmd_state == CMD_STATE_WAIT_CMD) {
			unsigned int data_len = uart_rx_buffered_data();
			if (data_len >= 13) {
//				asm volatile("cinv [d,i]");
//				for (volatile unsigned int i = 0; i < 1000; i++);
//				DMAX_CTRL_REG(DMA_UART_RX) &= ~DMAX_CTRL_REG_DMA_ON;
//				while (DMAX_CTRL_REG(DMA_UART_RX) & DMAX_CTRL_REG_DMA_ON);
				uint8_t *hdr = uart_get_read_ptr();
				uart_advance_read_ptr(13);
				uint8_t cmd = hdr[0];
				id = read_le32(&hdr[1]);
				parameter_len = read_le32(&hdr[5]);
				uint32_t crc_check = crc32_init();
				crc_check = crc32_update(crc_check, hdr, 9);
				crc_check = crc32_final(crc_check);
				uint32_t crc = read_le32(&hdr[9]);
//				DMAX_CTRL_REG(DMA_UART_RX) |= DMAX_CTRL_REG_DMA_ON;
/*
				debug_puts("CRC32 embedded ");
				debug_hexdump(hdr, 13);
				debug_puts("\r\n");
*/
				if (crc_check == crc) {
					if (cmd < ARRAY_SIZE(cmd_handlers)) {
						current_handler = &cmd_handlers[cmd];
						if (parameter_len >= current_handler->min_param_len) {
							timeout = TIMEOUT_PARAM;
							cmd_state = CMD_STATE_WAIT_PARAM;
						} else {
							timeout = TIMEOUT_HEADER;
							cmd_state = CMD_STATE_WAIT_HEADER;
							send_response(RESPONSE_PARAM_SHORT, id);
							reset_uart_rx_dma();
						}
					} else {
						timeout = TIMEOUT_HEADER;
						cmd_state = CMD_STATE_WAIT_HEADER;
						send_response(RESPONSE_CMD_INVALID, id);
						reset_uart_rx_dma();
					}
				} else {
					debug_puts("Invalid CRC32 on header ");
					debug_hexdump(hdr, 13);
					debug_puts("\r\n");
					debug_puts("Expected 0x");
					debug_putlong_hex(crc_check);
					debug_puts(" but received 0x");
					debug_putlong_hex(crc);
					debug_puts("\r\n");
					timeout = TIMEOUT_HEADER;
					cmd_state = CMD_STATE_WAIT_HEADER;
					send_response(RESPONSE_INVALID_CRC, id);
					reset_uart_rx_dma();
				}
			}
		}

		if (cmd_state == CMD_STATE_WAIT_PARAM) {
			if (parameter_len) {
				unsigned int data_len = uart_rx_buffered_data();
				if (data_len >= parameter_len + 4) {
	//				asm volatile("cinv [d,i]");
	//				for (volatile unsigned int i = 0; i < 1000; i++);
	//				DMAX_CTRL_REG(DMA_UART_RX) &= ~DMAX_CTRL_REG_DMA_ON;
	//				while (DMAX_CTRL_REG(DMA_UART_RX) & DMAX_CTRL_REG_DMA_ON);
					const uint8_t *read_ptr = uart_get_read_ptr();
					uart_advance_read_ptr(parameter_len + 4);
					uint32_t crc_check = crc32_init();
					crc_check = crc32_update(crc_check, read_ptr, parameter_len);
					crc_check = crc32_final(crc_check);
					uint32_t crc = read_le32(&read_ptr[parameter_len]);
	//				DMAX_CTRL_REG(DMA_UART_RX) |= DMAX_CTRL_REG_DMA_ON;
					if (crc_check == crc) {
						dispatch_cmd(current_handler, id, read_ptr, parameter_len);
						reset_uart_rx_dma();
						cmd_state = CMD_STATE_WAIT_HEADER;
						timeout = TIMEOUT_HEADER;
					} else {
						debug_puts("Invalid CRC32 on params ");
						debug_hexdump(read_ptr, parameter_len + 4);
						debug_puts("\r\n");
						debug_puts("Expected 0x");
						debug_putlong_hex(crc_check);
						debug_puts(" but received 0x");
						debug_putlong_hex(crc);
						debug_puts("\r\n");
						timeout = TIMEOUT_HEADER;
						cmd_state = CMD_STATE_WAIT_HEADER;
						send_response(RESPONSE_INVALID_CRC, id);
						reset_uart_rx_dma();
					}
				}
			} else {
				dispatch_cmd(current_handler, id, NULL, 0);
				reset_uart_rx_dma();
				cmd_state = CMD_STATE_WAIT_HEADER;
				timeout = TIMEOUT_HEADER;
			}
		}

		if (timeout) {
			timeout--;
			watchdog_reset();
		} else {
			debug_puts("Timeout elapsed, resetting\r\n");
			system_reset();
		}
	}

	system_reset();

	while (1) { }
}

