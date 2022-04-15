        .text
        .code 32

	.global mul
	.global mulr
	.global muls
	.global mulsr

mul:	smull	r2,r0,r1,r0
	mov	pc,lr
	
muls:	smull	r2,r0,r1,r0
	movs	r2,r2,lsl #1
	adc	r0,r0,r0
	mov	pc,lr

mulr:	smull	r2,r0,r1,r0
	adds	r2,r2,#0x80000000
	adc	r0,r0,#0
	mov	pc,lr

mulsr:	smull	r2,r0,r1,r0
	movs	r2,r2,lsl #1
	adc	r0,r0,r0
	adds	r2,r2,#0x80000000
	adc	r0,r0,#0
	mov	pc,lr

	.end


