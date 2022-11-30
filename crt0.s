.section .text.crt0
.globl bootloader_entry
bootloader_entry:
	/* Setup user stack */
	movd $__ustack, (sp)
	/* Setup excpetion steack */
	movd $__istack, (r1, r0)
	lprd (r1, r0), isp
	/* Fixup exception handlers, need to be shifted right by 1 bit */
	bal (ra), vector_table_fixup
	/* Setup CPU to read exception handlers as 32 bit wide */
	spr cfg, r0
	orw $0x100, r0
	lpr r0, cfg
	/* Setup exception table base address */
	movd $__intbase, (r1, r0)
	lprd (r1, r0), intbase
	/* Enable interrupts locally */
	di
	/* Enable interrupts globally */
	spr psr, r0
	orw $0x800, r0
	lpr r0, psr
	/* Branch to C code */
	br _c_entry
