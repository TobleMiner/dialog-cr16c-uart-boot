.section .text.crt0
.globl bootloader_entry
bootloader_entry:
	movd $__ustack, (sp)
	movd $__istack, (r1, r0)
	lprd (r1, r0), isp
	spr cfg, r0
	orw $0x100, r0
	lpr r0, cfg
	movd $__intbase, (r1, r0)
	lprd (r1, r0), intbase
	br _c_entry
