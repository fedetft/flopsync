	.syntax unified
	.cpu cortex-m3
	.fpu softvfp
	.thumb
	.align	2

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@	
	
	.global	trigger_80ns
	.thumb
	.thumb_func
	.type	trigger_80ns, %function
trigger_80ns:
	.fnstart
	mov	r2, #3072
	movt	r2, 16385
	mov	r1, #512

	mov	r3, #2048
	movt	r3, 16385
	mov	r0, #4096

	str	r1, [r2, #16]
	str	r0, [r3, #16]
	bx	lr
	.cantunwind
	.fnend
	.size	trigger_80ns, .-trigger_80ns

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@	
	
	.global	trigger_160ns
	.thumb
	.thumb_func
	.type	trigger_160ns, %function
trigger_160ns:
	.fnstart
	mov	r2, #3072
	movt	r2, 16385
	mov	r1, #512

	mov	r3, #2048
	movt	r3, 16385
	mov	r0, #4096

	str	r1, [r2, #16]
	nop
	nop
	nop
	str	r0, [r3, #16]
	bx	lr
	.cantunwind
	.fnend
	.size	trigger_160ns, .-trigger_160ns
	
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@	
	
	.global	trigger_320ns
	.thumb
	.thumb_func
	.type	trigger_320ns, %function
trigger_320ns:
	.fnstart
	mov	r2, #3072
	movt	r2, 16385
	mov	r1, #512

	mov	r3, #2048
	movt	r3, 16385
	mov	r0, #4096

	str	r1, [r2, #16]
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	str	r0, [r3, #16]
	bx	lr
	.cantunwind
	.fnend
	.size	trigger_320ns, .-trigger_320ns
	
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@	
	
	.global	trigger_640ns
	.thumb
	.thumb_func
	.type	trigger_640ns, %function
trigger_640ns:
	.fnstart
	mov	r2, #3072
	movt	r2, 16385
	mov	r1, #512

	mov	r3, #2048
	movt	r3, 16385
	mov	r0, #4096

	str	r1, [r2, #16]
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	str	r0, [r3, #16]
	bx	lr
	.cantunwind
	.fnend
	.size	trigger_640ns, .-trigger_640ns