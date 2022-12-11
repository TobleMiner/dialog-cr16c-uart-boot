#pragma once

#include "util.h"

#define DEBUG_REG		MMIO16(0xFF5004)
#define DEBUG_REG_SW_RESET	(1 << 7)

__attribute__((noreturn))
void system_reset(void);
