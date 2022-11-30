#include "util.h"

#define SET_INT_PENDING_REG				MMIO16(0xFF5400)
#define RESET_INT_PENDING_REG				MMIO16(0xFF5402)
#define RESET_INT_PENDING_REG_UART_TI_INT_PEND		(1 << 5)
#define RESET_INT_PENDING_REG_UART_RI_INT_PEND		(1 << 4)
#define INT0_PRIORITY_REG				MMIO16(0xFF5404)
#define INT1_PRIORITY_REG				MMIO16(0xFF5406)
#define INT2_PRIORITY_REG				MMIO16(0xFF5408)
#define INT2_PRIORITY_REG_UART_TI_INT_PRIO_MASK		(7 << 4)
#define INT2_PRIORITY_REG_UART_TI_INT_PRIO_SHIFT	4
#define INT2_PRIORITY_REG_UART_RI_INT_PRIO_MASK		(7 << 0)
#define INT2_PRIORITY_REG_UART_RI_INT_PRIO_SHIFT	0
#define INT3_PRIORITY_REG				MMIO16(0xFF540A)

typedef struct vector_table {
	funcp_t reserved00;
	funcp_t nmi;
	funcp_t reserved02;
	funcp_t reserved03;
	funcp_t reserved04;
	funcp_t svc_trap;
	funcp_t dvz_trap;
	funcp_t flg_trap;
	funcp_t bpt_trap;
	funcp_t trc_trap;
	funcp_t und_trap;
	funcp_t reserved11;
	funcp_t iad_trap;
	funcp_t reserved13;
	funcp_t dbg_trap;
	funcp_t ise_int;
	funcp_t access12_int;
	funcp_t keyb_int;
	funcp_t reserved_int18;
	funcp_t ct_classd_int;
	funcp_t uart_ri_int;
	funcp_t uart_ti_int;
	funcp_t spi_int;
	funcp_t tim0_int;
	funcp_t tim1_int;
	funcp_t clk100_int;
	funcp_t dip_int;
	funcp_t ad_int;
	funcp_t spi2;
	funcp_t dsp_int;
	funcp_t reserved30;
} vector_table_t;
