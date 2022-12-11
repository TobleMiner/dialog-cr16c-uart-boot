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

.align 4
/* Vector dispatch to C code */
vector_common:
	movd $_vector_table, (r3, r2)
	addd (r1, r0), (r3, r2)
	loadd 0x0(r3, r2), (r1, r0)
	jal (ra), (r1, r0)
	pop $0x4, r0, ra
	retx

/* Enable eval (%<expr>) */
.altmacro

/* Define vectors */
.macro vectors start=0, end=31, offset=0
vector\start:
	push $0x4, r0, ra
.if \start == 5
	/*
	 * This was a SVC, SVC has fixed instruction length of 2.
	 * Point stored PC to instruction after SVC
	 */
	sprd isp, (r1, r0)
	loadd 0x00(r1, r0), (r3, r2)
	addd $1, (r3, r2)
	stord (r3, r2), 0x0(r1, r0)
.endif
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

