#include "crc32.h"

#include "util.h"

#define CRC32_POLY 0xedb88320

static uint32_t crc32_table[256];

void crc32_populate_table(void) __attribute__((constructor));
void crc32_populate_table(void) {
	unsigned int byt, bit;

	for (byt = 0; byt < ARRAY_SIZE(crc32_table); byt++) {
		uint32_t crc = byt;

		for (bit = 0; bit < 8; bit++) {
			if (crc & 1) {
				crc >>= 1;
				crc ^= CRC32_POLY;
			} else {
				crc >>= 1;
			}
		}
		crc32_table[byt] = crc;
	}
}

uint32_t crc32_init() {
	return 0xFFFFFFFFUL;
}

uint32_t crc32_update(uint32_t crc, const void *data, unsigned int len) {
	const uint8_t *data8 = data;

	while (len--) {
		unsigned int idx = (crc ^ *data8++) & 0xff;

		crc = (crc >> 8) ^ crc32_table[idx];
	}

	return crc;
}

uint32_t crc32_final(uint32_t crc) {
	return ~crc;
}
