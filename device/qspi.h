#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "util.h"

#define QSPIC_CTRL_REG			MMIO32(0xFF0C00)
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

#define QSPIC_DEASSERT_CS()	QSPIC_CTRL_REG = QSPIC_CTRL_REG_DISABLE_BUS

#define QSPIC_WAIT_NOT_BUSY()	do { } while (QSPIC_STATUS_REG & QSPIC_STATUS_REG_BUSY)

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

typedef enum {
	QSPI_READ,
	QSPI_WRITE,
	QSPI_DUMMY
} qspi_action_t;

typedef struct qspi_xfer_action {
	qspi_action_t action;
	union {
		const void *tx_data;
		void *rx_data;
	};
	size_t len;
} qpsi_xfer_action_t;

void qspi_init(void);
void qspi_write_then_read(const qspi_xfer_desc_t *desc);
void qspi_scatter_transfer(qspi_mode_t mode, const qpsi_xfer_action_t *actions, unsigned int num_actions);
void qspi_set_write_protect(bool protection_on);

