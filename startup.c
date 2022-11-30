#include "dma.h"
#include "util.h"

extern unsigned int _data_loadaddr;
extern unsigned int _data;
extern unsigned int _ebss;
extern unsigned int _edata;
extern unsigned int _stack;

extern funcp_t _preinit_array_start, _preinit_array_end;
extern funcp_t _init_array_start, _init_array_end;

int main(void);

void c_entry(void) __attribute__((noreturn));
void c_entry() {
	volatile unsigned *src, *dest;
	for (src = &_data_loadaddr, dest = &_data; dest < &_edata; src++, dest++) {
		*dest = *src;
	}

	while (dest < &_ebss) {
		*dest++ = 0;
	}

	/* Call constructors. */
	funcp_t *fp;
	for (fp = &_preinit_array_start; fp < &_preinit_array_end; fp++) {
		(*fp)();
	}
	for (fp = &_init_array_start; fp < &_init_array_end; fp++) {
		(*fp)();
	}

	/* Enable interrupts */
	dma_disable_irq_rerouting();
	asm("ei");

	(void)main();

	while (1);
}

