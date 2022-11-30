#pragma once

#include "util.h"

#define DMA0_CTRL_REG		MMIO16(0xFF440C)
#define DMA1_CTRL_REG		MMIO16(0xFF441C)
#define DMA2_CTRL_REG		MMIO16(0xFF442C)
#define DMA3_CTRL_REG		MMIO16(0xFF443C)
#define DMAX_CTRL_REG_DINT_MODE	(1 << 3)

void dma_disable_irq_rerouting(void);

