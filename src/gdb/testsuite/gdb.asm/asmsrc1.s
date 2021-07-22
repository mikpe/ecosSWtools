; "main" routine for assembly source debugging test
; Eventually this code needs to be made more machine independent
; (with the actual code coming from macros in some header file)
; so that the same driver will work for several architectures.
;
; WARNING: asm-source.exp checks for line numbers printed by gdb,
; therefore be careful about changing this file without also changing
; asm-source.exp.

	.macro exit0
	ldi r4, 1
	ldi r0, 0
	trap 15
	.endm

	.macro several_nops
	nop
	nop
	nop
	nop
	.endm

; Program begins here.

	.global main
main:
	st r13,@-sp

; Call a macro that consists of several lines of assembler code.

	several_nops

; Call a subroutine in another file.

	bl foo2

; All done.

	exit0

; A routine for foo2 to call.

	.global foo3
foo3:
	st r13,@-sp

	ld r13,@sp+
	jmp r13

	.global exit
exit:
	exit0
