	.file "setjmp.S"

	.section .text
	.align 1
	.global _setjmp
_setjmp:
        mov d0,a0
        mov d2,(0,a0)
        mov d3,(4,a0)
        mov a2,(12,a0)
        mov a3,(16,a0)
	mov sp,(20,a0)
        sub d0,d0
	rets

	.global _longjmp
_longjmp:
	mov d0,a0
        mov (0,a0),d2
        mov (4,a0),d3
        mov (12,a0),a2
        mov (16,a0),a3
	mov (20,a0),sp
	mov 1,d0
	rets
