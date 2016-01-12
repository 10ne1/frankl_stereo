
	.text
	.align	4

	.global	memcp_vfp
	.type	memcp_vfp, %function
memcp_vfp:
	vldm r1!, {s0}
	vstm r0!, {s0}
	subs r2, r2,  #4
	bgt memcp_vfp
	mov pc, lr

	.text
	.align	4

	.global	memcp_vfpX
	.type	memcp_vfpX, %function
memcp_vfpX:
	mov r3, #0
	vdup.32 d0, r3
        mov r3, #0xFFFFFFFF
	vdup.32 d1, r3
	mov r3, #0
	vdup.32 d2, r3
	vldm r1!, {d0}
	veor d2, d0, d1
	vdup.32 d0, r3
	veor d0, d2, d1
	vstm r0!, {d0}
	subs r2, r2,  #8
	bgt memcp_vfpX
	mov pc, lr



	.text
	.align	4

	.global	memclean_vfpX
	.type	memclean_vfpX, %function
memclean_vfpX:
	mov r3, #0
	mov r3, #0
	vdup.32 d0, r3
	vstm r0!, {d0}
	subs r1, r1,  #8
	bgt memclean_vfpX
	mov pc, lr

