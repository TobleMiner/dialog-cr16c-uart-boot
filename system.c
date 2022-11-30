#include "system.h"

void system_reset(void) {
	DEBUG_REG |= DEBUG_REG_SW_RESET;
	while(1);
}

