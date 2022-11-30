#include "qspi.h"

#include "clock.h"

void qspi_init() {
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
	QSPIC_CTRL_REG = QSPIC_CTRL_REG_SIO_EN;
	/* QSPI IO config, release reset, DO idle level high */
	/* 0x20 = RST/IO3 high */
	/* 0x10 = WP/IO2 high */
	/* 0x08 = RST/IO3 output enable */
	/* 0x04 = WP/IO2 output enable */
	/* 0x02 = DO/IO0 and CLK idle level high */
	/* 0x01 = auto mode */
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
	QSPIC_CTRL_REG = QSPIC_CTRL_REG_QIO_EN;
	QSPIC_WRITEDATA8_REG = 0xff;
	QSPIC_WAIT_NOT_BUSY();

	QSPIC_CTRL_REG = QSPIC_CTRL_REG_DIO_EN;
	QSPIC_WRITEDATA8_REG = 0xff;
	QSPIC_WAIT_NOT_BUSY();

	QSPIC_CTRL_REG = QSPIC_CTRL_REG_SIO_EN;
	qspic_deassert_reassert_cs();
	QSPIC_WRITEDATA8_REG = 0xf5;
	QSPIC_WAIT_NOT_BUSY();

	QSPIC_CTRL_REG = QSPIC_CTRL_REG_DISABLE_BUS;
//	qspic_deassert_reassert_cs();
//	QSPIC_WRITEDATA8_REG = 0xaa;
	QSPIC_CTRL_REG = QSPIC_CTRL_REG_QIO_EN;
	QSPIC_WRITEDATA16_REG = 0xaa55;
	QSPIC_WAIT_NOT_BUSY();

	qspic_deassert_reassert_cs();

	QSPIC_CTRL_REG = QSPIC_CTRL_REG_QIO_EN;
	QSPIC_WRITEDATA16_REG = 0xaa55;
	QSPIC_WAIT_NOT_BUSY();

	QSPIC_CTRL_REG = QSPIC_CTRL_REG_DISABLE_BUS;
*/

}

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
			uint32_t datum = 0;
			int i;
			for (i = 0; i < 4; i++) {
				if (data_len) {
					datum |= (((uint32_t)*data++) << (i * 8));
					data_len--;
				} else {
					datum |= (((uint32_t)0xff) << (i * 8));
					dummy_len--;
				}
			}
			QSPIC_WAIT_NOT_BUSY();
			QSPIC_WRITEDATA32_REG = datum;
		} else if (combined_len >= 2) {
			uint16_t datum = 0;
			int i;
			for (i = 0; i < 2; i++) {
				if (data_len) {
					datum |= (((uint16_t)*data++) << (i * 8));
					data_len--;
				} else {
					datum |= (((uint16_t)0xff) << (i * 8));
					dummy_len--;
				}
			}
			QSPIC_WAIT_NOT_BUSY();
			QSPIC_WRITEDATA16_REG = datum;
		} else {
			QSPIC_WAIT_NOT_BUSY();
			if (data_len) {
				QSPIC_WRITEDATA8_REG = *data++;
				data_len--;
			} else {
				QSPIC_WRITEDATA8_REG = 0xff;
				dummy_len--;
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

void qspi_write_then_read(const qspi_xfer_desc_t *desc) {
	switch (desc->mode) {
	case QSPI_MODE_SIO:
		QSPIC_CTRL_REG = QSPIC_CTRL_REG_SIO_EN;
		break;
	case QSPI_MODE_DIO:
		QSPIC_CTRL_REG = QSPIC_CTRL_REG_DIO_EN;
		break;
	case QSPI_MODE_QIO:
		QSPIC_CTRL_REG = QSPIC_CTRL_REG_QIO_EN;
		break;
	}

	QSPIC_CTRL_REG = QSPIC_CTRL_REG_ENABLE_BUS;
	if ((desc->tx_data && desc->tx_len) || desc->dummy_cycles_after_tx) {
		qspi_tx(desc);
	}
	if (desc->rx_data && desc->rx_len) {
		qspi_rx(desc);
	}
	QSPIC_CTRL_REG = QSPIC_CTRL_REG_DISABLE_BUS;
}

