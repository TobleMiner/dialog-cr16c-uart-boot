#pragma once

#define WATCHDOG_REG MMIO16(0xFF4C00)

static inline void watchdog_reset(void) {
	WATCHDOG_REG = 0xff;
}
