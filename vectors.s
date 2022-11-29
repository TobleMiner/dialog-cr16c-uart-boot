.section .text
.align 4
.global vector_table_fixup
vector_table_fixup:
	movd $_real_vector_table, (r1, r0)
	movw $31, r2
fixup_exception_entry:
	loadd 0x0(r1, r0), (r5, r4)
	lshd $-1, (r5, r4)
	stord (r5, r4), 0x0(r1, r0)
	addd $4, (r1, r0)
	cmpw $0, r2
	subw $1, r2
	bne fixup_exception_entry
	jump (ra)

.macro io2_on
	sbitw $4, 0xFF0C04
	sbitw $2, 0xFF0C04
.endm

.macro io2_off
	cbitw $2, 0xFF0C04
	cbitw $4, 0xFF0C04
.endm

.macro io3_on
	sbitw $5, 0xFF0C04
	sbitw $3, 0xFF0C04
.endm

.macro io3_off
	cbitw $3, 0xFF0C04
	cbitw $5, 0xFF0C04
.endm

/* b in r2 */
uart_putb:
	push $4, r0
	io3_on
	storb r2, 0xFF4902
uart_wait_done:
	tbitw $5, 0xFF4900
	bfc uart_wait_done
	sbitw $1, 0xFF4904
	io3_off
	popret $4, r0

/* d in r3,r2 */
uart_putd:
	push $4, r0, ra
	bal (ra), uart_putb
	lshd $-8, (r3, r2)
	bal (ra), uart_putb
	lshd $-8, (r3, r2)
	bal (ra), uart_putb
	lshd $-8, (r3, r2)
	bal (ra), uart_putb
	popret $4, r0, ra

.align 4
teststring:
.asciz "Hello world from C called through assembly"

newline:
.asciz "\r\n"

unsupportedbase:
.asciz "unsupported base"

.macro uart_print sym
push $4, r0, ra
movd $\sym, (r3, r2)
bal (ra), _uart_puts
pop $4, r0, ra
.endm

.macro uart_newline
uart_print newline
.endm

.macro uart_print_int reg base=16
push $4, r0, ra
movw \reg, r2
.if base == 16
bal (ra), _uart_putint_hex
.elseif base == 10
bal (ra), _uart_putint
.else
uart_print unsupportedbase
bal (ra), _uart_putint_hex
.endif
pop $4, r0, ra
.endm

.macro uart_print_long reg1 reg2 base=16
push $4, r0, ra
movw \reg1, r2
movw \reg2, r3
.if base == 16
bal (ra), _uart_putlong_hex
.elseif base == 10
bal (ra), _uart_putlong
.else
uart_print unsupportedbase
bal (ra), _uart_putlong_hex
.endif
pop $4, r0, ra
.endm

handlerstring:
.asciz "C handler is located at 0x"

handleroffsetstring:
.asciz "C handler offset 0x"

handlertablestring:
.asciz "C handler table is @ 0x"

.align 4
/* Vector dispatch to C code */
vector_common:
	uart_print handleroffsetstring
	uart_print_long r0 r1 16
	uart_newline

	movd $_vector_table, (r3, r2)
	uart_print handlertablestring
	uart_print_long r2 r3 16
	uart_newline

	addd (r1, r0), (r3, r2)
	loadd 0x0(r3, r2), (r1, r0)

	uart_print handlerstring
	uart_print_long r0 r1 16
	uart_newline

	jal (ra), (r1, r0)
	pop $0x4, r0, ra
	retx

/* Enable eval (%<expr>) */
.altmacro

pcstring:
.asciz "PC is pointed at 0x"

spstring:
.asciz "SP is pointed at 0x"

uspstring:
.asciz "USP is pointed at 0x"

ispstring:
.asciz "ISP is pointed at 0x"

storedpcstring:
.asciz "stored PC is pointed at 0x"

vectorstring:
.asciz "Executing vector "

.align 4
/* Define vectors */
.macro vectors start=0, end=31, offset=0
vector\start:
	/* Exception entry, PC it on top of stack */
	io2_on
	push $4, r0, ra
	/* After push, r0, r1, r2, r3 and ra are now on stack, PC @(SP - 8) */
	uart_print vectorstring
	movw $\start, r2
	uart_print_int r2 10
	uart_newline

	uart_print pcstring
	loadd 0x08(sp), (r3, r2)
	lshd $1, (r3, r2)
	uart_print_long r2 r3
	uart_newline

	uart_print spstring
	movd (sp), (r1, r0)
	uart_print_long r0 r1
	uart_newline

	uart_print uspstring
	sprd usp, (r1, r0)
	uart_print_long r0, r1
	uart_newline

	uart_print ispstring
	sprd isp, (r1, r0)
	uart_print_long r0 r1
	uart_newline

	uart_print storedpcstring
	loadd 0x00(r1, r0), (r3, r2)
#	lshd $1, (r3, r2)
	uart_print_long r2 r3
	uart_newline


	/*
	 * This was a SVC, SVC has fixed instruction length of 2.
	 * Point stored PC to instruction after SVC
	 */
	loadd 0x00(r1, r0), (r3, r2)
	addd $1, (r3, r2)
	stord (r3, r2), 0x0(r1, r0)
	pop $4, r0, ra
	io2_off

	push $0x4, r0, ra
	movd $\offset, (r1, r0)
	br vector_common
.if \start-\end
vectors %(\start+1), \end, %((\start+1)*4)
.endif
.endm

vectors

/* Define vector table */
.macro vector_table_entries start=0, end=31
.dc.l vector\start
.if \start-\end
vector_table_entries %(\start+1), \end
.endif
.endm

.section .vectors
.align 4
.globl _real_vector_table
_real_vector_table:
	vector_table_entries

