#pragma once

#include <stdint.h>

uint32_t crc32_init(void);
uint32_t crc32_update(uint32_t crc, const void *data, unsigned int len);
uint32_t crc32_final(uint32_t crc);
