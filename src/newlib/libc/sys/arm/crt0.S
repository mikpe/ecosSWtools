#include "swi.h"

/* ANSI concatenation macros.  */
#define CONCAT(a, b) CONCAT2(a, b)
#define CONCAT2(a, b) a ## b

#ifdef __USER_LABEL_PREFIX__
#define FUNCTION( name ) CONCAT (__USER_LABEL_PREFIX__, name)
#else
#define	FUNCTION( name ) CONCAT ( _ , name)
#endif

/* .text is used instead of .section .text so it works with arm-aout too.  */
	.text
	.code 32
	.align 	0

	.global	_mainCRTStartup
	.global	_start
	.global	start
start:
_start:
_mainCRTStartup:

/* Start by setting up a stack */
#ifdef ARM_RDP_MONITOR
	/*  Issue Demon SWI to read stack info */
	swi	SWI_GetEnv	/*  Returns command line in r0 */
	mov	sp,r1		/*  and the highest memory address in r1 */
	ldr	sl, LC2		/*  stack limit is at end of data */
	add	sl, sl, #256	/*  allow slop for stack overflow handling */
				/*  and small frames */
#else
#ifdef ARM_RDI_MONITOR
	/*  Issue Angel SWI to read stack info */
	mov	r0, #AngelSWI_Reason_HeapInfo
	adr	r1, LC0		/*  point at ptr to 4 words to receive data */
	swi	AngelSWI_ARM	/*  We are always in ARM mode for startup */
	ldr	r0, LC0		/*  point at values read */
	ldr	sp, [r0, #8]
	ldr	sl, [r0, #12]
	add	sl, sl, #256	/*  allow slop for stack overflow handling */
				/*  and small frames */
#else
	/*  Set up the stack pointer to a fixed value */
	ldr	r3, LC0
	mov 	sp, r3
	/* Setup a default stack-limit in-case the code has been
	   compiled with "-mapcs-stack-check".  Hard-wiring this value
	   is not ideal, since there is currently no support for
	   checking that the heap and stack have not collided, or that
	   this default 64k is enough for the program being executed.
	   However, it ensures that this simple crt0 world will not
	   immediately cause an overflow event:  */
	sub	sl, sp, #64 << 10	/* Still assumes 256bytes below sl */
#endif
#endif
	mov 	a2, #0			/* Second arg: fill value */
	mov	fp, a2			/* Null frame pointer */
	mov	r7, a2			/* Null frame pointer for Thumb */
	
	ldr	a1, LC1			/* First arg: start of memory block */
	ldr	a3, LC2	
	sub	a3, a3, a1		/* Third arg: length of block */
	

#ifdef __thumb__		/* Enter Thumb mode.... */

	add	a4, pc, #1	/* Get the address of the Thumb block */
	bx	a4		/* Go there and start Thumb decoding  */

	.code 16
	.global change_mode
change_mode:	
#endif
	
	bl	FUNCTION (memset)
#if !defined (ARM_RDP_MONITOR) && !defined (ARM_RDI_MONITOR)
	mov	r0, #0		/*  no arguments  */
	mov	r1, #0		/*  no argv either */
#else
#ifdef ARM_RDP_MONITOR
	swi	SWI_GetEnv	/*  sets r0 to point to the command line */
	mov	r1, r0
#else
	/* Need to set up standard file handles */
	bl	FUNCTION (initialise_angel_handles)
	mov	r0, #AngelSWI_Reason_GetCmdLine
	adr	r1, LC30	/*  Space for command line */
	swi	AngelSWI
	ldr	r1, LC30
#endif
	/*  Parse string at r1 */
	mov	r0, #0		/*  count of arguments so far */
LC10:
/*  Skip leading blanks */
#ifdef __thumb__
	ldrb	r3, [r1]
	add	r1, #1
#else
	ldrb	r3, [r1], #1
#endif
	cmp	r3, #0
	beq	LC12
	cmp	r3, #' '
	beq	LC10

/*  See whether we are scanning a string */
	cmp	r3, #'"'
#ifdef __thumb__
	beq	LC20
	cmp	r3, #'\''
	bne	LC21
LC20:
	mov	r2, r3
	b	LC22

LC21:
	mov	r2, #' '	/*  terminator type */
	sub	r1, r1, #1	/*  adjust back to point at start char */
LC22:
#else
	cmpne	r3, #'\''
	moveq	r2, r3
	movne	r2, #' '	/*  terminator type */
	subne	r1, r1, #1	/*  adjust back to point at start char */
#endif

/*  Stack a pointer to the current argument */
#ifdef __thumb__
	push	{r1}
#else
	stmfd	sp!, {r1}
#endif
	add	r0, r0, #1
LC11:
#ifdef __thumb__
	ldrb	r3, [r1]
	add	r1, #1
#else
	ldrb	r3, [r1], #1
#endif
	cmp	r3, #0
	beq	LC12
	cmp	r2, r3		/*  reached terminator? */
	bne	LC11
	mov	r2, #0
	sub	r3, r1, #1
	strb	r2, [r3]	/*  terminate the arg string */
	b	LC10

LC12:
	mov	r1, sp		/*  point at stacked arg pointers */
	/* We've now got the stacked args in order reverse the */
#ifdef __thumb__
	mov	r2, r0
	lsl	r2, #2
	add	r2, sp
	mov	r3, sp
LC15:	cmp	r2, r3
	bls	LC14
	sub	r2, #4
	ldr	r4, [r2]
	ldr	r5, [r3]
	str	r5, [r2]
	str	r4, [r3]
	add	r3, #4
	b	LC15
LC14:	
#else
	add	r2, sp, r0, LSL #2	/* End of args */
	mov	r3, sp			/* Start of args */
LC13:	cmp	r2, r3
	ldrhi	r4,[r2, #-4]		/* Reverse ends of list */
	ldrhi	r5, [r3]
	strhi	r5, [r2, #-4]!
	strhi	r4, [r3], #4
	bhi	LC13
#endif

#endif
	bl	FUNCTION (main)

	bl	FUNCTION (exit)		/* Should not return */

#ifdef __thumb__
	/* Come out of Thumb mode... This code should be redundant...   */

	mov	a4, pc
	bx	a4

	.code 32
	.global change_back
change_back:
	/* Halt the execution.  This code should never be executed.  */
	/*
	** With no debug monitor, this probably aborts (eventually).
	** With a Demon debug monitor, this halts cleanly.
	** With an Angel debug monitor, this will report 'Unknown SWI'.
	*/
	swi	SWI_Exit
#endif
	
	/* For Thumb, constants must be after the code since only 
	positive offsets are supported for PC relative addresses. */
	
	.align 0
LC0:
#ifdef ARM_RDI_MONITOR
	.word	HeapBase
#else
#ifndef ARM_RDP_MONITOR
#ifdef __pe__
	.word	0x800000
#else
/*	.word	0x80000	*/		/* Top of RAM on the PIE board */
#endif
#endif
#endif
LC1:
	.word	__bss_start__
LC2:
	.word	__bss_end__

#ifdef ARM_RDI_MONITOR
LC30:	.word	CommandLine

/*  Workspace for Angel calls. */
	.data
/*  Data returned by monitor SWI */
HeapBase:	.word	0
HeapLimit:	.word	0
StackBase:	.word	0
StackLimit:	.word	0
CommandLine:	.space	256,0	/*  Maximum length of 255 chars handled */
#endif
	
#ifdef __pe__
	.section .idata$3
	.long	0,0,0,0,0,0,0,0
#endif

