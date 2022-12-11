#pragma once

#include <stdint.h>

#define MMIO16(addr) (*((volatile uint16_t*)(addr)))
#define MMIO32(addr) (*((volatile uint32_t*)(addr)))
#define MMIO8(addr) (*((volatile uint8_t*)(addr)))

#define ARRAY_SIZE(x_) (sizeof(x_) / sizeof(*(x_)))

typedef void (*funcp_t) (void);
