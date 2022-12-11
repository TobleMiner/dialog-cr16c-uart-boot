#include "chipid.h"

void chipid_read(chipid_t *id) {
	id->id1 = CHIP_ID1_REG;
	id->id2 = CHIP_ID2_REG;
	id->id3 = CHIP_ID3_REG;
	id->mem_size = CHIP_MEM_SIZE_REG;
	id->revision = CHIP_REVISION_REG;
}
