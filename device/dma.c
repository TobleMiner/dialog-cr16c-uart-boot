#include "dma.h"

void dma_disable_irq_rerouting() {
	DMA0_CTRL_REG &= ~DMAX_CTRL_REG_DINT_MODE;
	DMA1_CTRL_REG &= ~DMAX_CTRL_REG_DINT_MODE;
	DMA2_CTRL_REG &= ~DMAX_CTRL_REG_DINT_MODE;
	DMA3_CTRL_REG &= ~DMAX_CTRL_REG_DINT_MODE;
}
