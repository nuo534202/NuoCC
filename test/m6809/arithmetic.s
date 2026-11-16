wide:
	.word	0
	.word	0
not_wide:
	.word	0
	.word	0
small:
	.byte	0
main:
	pshs	u
	leas	-82,s
	leau	,s
	ldd	#1
	std	2,u
	ldd	#0
	std	4,u
	ldd	2,u
	std	wide
	ldd	4,u
	std	wide+2
	ldd	wide
	std	2,u
	ldd	wide+2
	std	4,u
	ldd	2,u
	lbne	__Larithmetic_5
	ldd	4,u
	lbeq	__Larithmetic_3
__Larithmetic_5:
	ldd	#1
	std	4,u
	jmp	__Larithmetic_4
__Larithmetic_3:
	ldd	#0
	std	4,u
__Larithmetic_4:
	ldd	4,u
	lbeq	__Larithmetic_2
	ldd	#1
	std	4,u
	jsr	__printint
__Larithmetic_2:
	ldd	#0
	std	2,u
	ldd	#1
	std	4,u
	ldd	2,u
	std	wide
	ldd	4,u
	std	wide+2
	ldd	wide
	std	2,u
	ldd	wide+2
	std	4,u
	ldd	2,u
	lbne	__Larithmetic_9
	ldd	4,u
	lbeq	__Larithmetic_7
__Larithmetic_9:
	ldd	#1
	std	4,u
	jmp	__Larithmetic_8
__Larithmetic_7:
	ldd	#0
	std	4,u
__Larithmetic_8:
	ldd	4,u
	lbeq	__Larithmetic_6
	ldd	#2
	std	4,u
	jsr	__printint
__Larithmetic_6:
	ldd	#0
	std	2,u
	ldd	#0
	std	4,u
	ldd	2,u
	std	wide
	ldd	4,u
	std	wide+2
	ldd	wide
	std	2,u
	ldd	wide+2
	std	4,u
	ldd	2,u
	lbne	__Larithmetic_14
	ldd	4,u
	lbeq	__Larithmetic_12
__Larithmetic_14:
	ldd	#1
	std	4,u
	jmp	__Larithmetic_13
__Larithmetic_12:
	ldd	#0
	std	4,u
__Larithmetic_13:
	ldd	4,u
	lbeq	__Larithmetic_10
	ldd	#3
	std	4,u
	jsr	__printint
	jmp	__Larithmetic_11
__Larithmetic_10:
	ldd	#4
	std	4,u
	jsr	__printint
__Larithmetic_11:
	ldd	#255
	std	4,u
	ldb	5,u
	stb	small
	clra
	ldb	small
	std	4,u
	std	8,u
	addd	#1
	clra
	std	8,u
	ldb	9,u
	stb	small
	ldd	8,u
	jsr	__printint
	clra
	ldb	small
	std	4,u
	jsr	__printint
	ldd	#0
	std	4,u
	ldb	5,u
	stb	small
	clra
	ldb	small
	std	4,u
	std	8,u
	addd	#65535
	clra
	std	8,u
	ldb	9,u
	stb	small
	ldd	8,u
	jsr	__printint
	clra
	ldb	small
	std	4,u
	jsr	__printint
	ldd	#200
	std	4,u
	ldb	5,u
	stb	small
	clra
	ldb	small
	std	4,u
	ldd	#3
	std	8,u
	leax	4,u
	leay	8,u
	jsr	__mul16
	clr	4,u
	ldd	4,u
	jsr	__printint
	clra
	ldb	small
	std	4,u
	ldd	#1
	std	8,u
	ldb	9,u
	lbeq	__Larithmetic_16
__Larithmetic_15:
	lsl	5,u
	decb
	lbne	__Larithmetic_15
__Larithmetic_16:
	ldd	4,u
	jsr	__printint
	ldx	#small
	stx	4,u
	ldd	4,u
	std	0,u
	ldd	#255
	std	4,u
	ldb	5,u
	stb	small
	ldd	0,u
	std	4,u
	ldd	#0
	std	8,u
	ldd	4,u
	addd	8,u
	std	4,u
	ldx	4,u
	clra
	ldb	,x
	std	8,u
	std	12,u
	ldd	#1
	std	16,u
	ldd	8,u
	addd	16,u
	clra
	std	8,u
	ldx	4,u
	ldb	9,u
	stb	,x
	ldd	12,u
	jsr	__printint
	clra
	ldb	small
	std	4,u
	jsr	__printint
	ldd	#0
	std	4,u
	ldb	5,u
	stb	small
	ldd	0,u
	std	4,u
	ldd	#0
	std	8,u
	ldd	4,u
	addd	8,u
	std	4,u
	ldx	4,u
	clra
	ldb	,x
	std	8,u
	std	12,u
	ldd	#1
	std	16,u
	ldd	8,u
	subd	16,u
	clra
	std	8,u
	ldx	4,u
	ldb	9,u
	stb	,x
	ldd	12,u
	jsr	__printint
	clra
	ldb	small
	std	4,u
	jsr	__printint
	ldd	#1
	std	2,u
	ldd	#0
	std	4,u
	ldd	2,u
	std	wide
	ldd	4,u
	std	wide+2
	ldd	wide
	std	2,u
	ldd	wide+2
	std	4,u
	ldd	2,u
	lbne	__Larithmetic_19
	ldd	4,u
	lbeq	__Larithmetic_17
__Larithmetic_19:
	ldd	#0
	std	4,u
	jmp	__Larithmetic_18
__Larithmetic_17:
	ldd	#1
	std	4,u
__Larithmetic_18:
	ldd	4,u
	lbpl	__Larithmetic_20
	ldd	#65535
	std	2,u
	jmp	__Larithmetic_21
__Larithmetic_20:
	ldd	#0
	std	2,u
__Larithmetic_21:
	ldd	2,u
	std	not_wide
	ldd	4,u
	std	not_wide+2
	ldd	not_wide
	std	2,u
	ldd	not_wide+2
	std	4,u
	ldd	#0
	std	6,u
	ldd	#0
	std	8,u
	ldd	2,u
	cmpd	6,u
	lbne	__Larithmetic_22
	ldd	4,u
	cmpd	8,u
	lbne	__Larithmetic_22
	ldd	#5
	std	4,u
	jsr	__printint
__Larithmetic_22:
	ldd	#4148
	std	2,u
	ldd	#65535
	std	4,u
	ldd	2,u
	std	wide
	ldd	4,u
	std	wide+2
	ldd	wide
	std	2,u
	ldd	wide+2
	std	4,u
	ldd	2,u
	std	6,u
	ldd	4,u
	std	8,u
	addd	#1
	std	8,u
	ldd	6,u
	adcb	#0
	adca	#0
	std	6,u
	std	wide
	ldd	8,u
	std	wide+2
	ldd	wide
	std	2,u
	ldd	wide+2
	std	4,u
	ldd	#4149
	std	6,u
	ldd	#0
	std	8,u
	ldd	2,u
	cmpd	6,u
	lbne	__Larithmetic_23
	ldd	4,u
	cmpd	8,u
	lbne	__Larithmetic_23
	ldd	#6
	std	4,u
	jsr	__printint
__Larithmetic_23:
	ldd	wide
	std	2,u
	ldd	wide+2
	std	4,u
	ldd	2,u
	std	6,u
	ldd	4,u
	std	8,u
	addd	#65535
	std	8,u
	ldd	6,u
	adcb	#255
	adca	#255
	std	6,u
	std	wide
	ldd	8,u
	std	wide+2
	ldd	wide
	std	2,u
	ldd	wide+2
	std	4,u
	ldd	#4148
	std	6,u
	ldd	#65535
	std	8,u
	ldd	2,u
	cmpd	6,u
	lbne	__Larithmetic_24
	ldd	4,u
	cmpd	8,u
	lbne	__Larithmetic_24
	ldd	#7
	std	4,u
	jsr	__printint
__Larithmetic_24:
	ldd	#0
	std	4,u
	std	__ret
__Larithmetic_1:
	leas	82,s
	puls	u
	rts
