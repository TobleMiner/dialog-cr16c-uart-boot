#pragma once

#include <stdint.h>

#include "util.h"

#define CHIP_ID1_REG		MMIO8(0xFFFBF8)
#define CHIP_ID2_REG		MMIO8(0xFFFBF9)
#define CHIP_ID3_REG		MMIO8(0xFFFBFA)
#define CHIP_MEM_SIZE_REG	MMIO8(0xFFFBFB)
#define CHIP_REVISION_REG	MMIO8(0xFFFBFC)

typedef struct chipid {
	uint8_t id1;
	uint8_t id2;
	uint8_t id3;
	uint8_t mem_size;
	uint8_t revision;
} chipid_t;

void chipid_read(chipid_t *id);
