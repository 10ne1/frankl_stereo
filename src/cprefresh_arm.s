	.text
	.align	4

	.global	memcp_reg
	.type	memcp_reg, %function
memcp_reg:
	ldr r3, [r1], #4
	str r3, [r0], #4
	subs r2, r2,  #4
        bgt memcp_reg
	mov pc, lr

        .text
        .align  4

        .global memcp_regX
        .type   memcp_regX, %function
memcp_regX:
        mov r3, #0
        mov r4, #0
        ldr r3, [r1], #4
        mov r12, #0xFFFFFFFF
        eor r4, r3, r12
        mov r12, #0xFFFFFFFF
        mov r3, #0
        eor r3, r4, r12
        str r3, [r0], #4
        subs r2, r2,  #4
        bgt memcp_regX
        mov pc, lr

        .text
        .align  4

        .global memclean_reg
        .type   memclean_reg, %function
memclean_reg:
        mov r3, #0
        mov r3, #0
        str r3, [r0], #4
        subs r1, r1,  #4
        bgt memclean_reg
        mov pc, lr

