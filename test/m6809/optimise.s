thing:
	rmb	4
twice:
	pshs	u
	leas	-82,s
	leau	,s
	ldd	86,u
	std	0,u
	ldd	0,u
	std	4,u
	ldd	#1
	std	8,u
	ldb	9,u
	lbeq	__Loptimise_3
__Loptimise_2:
	lsl	5,u
	rol	4,u
	decb
	lbne	__Loptimise_2
__Loptimise_3:
	ldd	4,u
	std	__ret
__Loptimise_1:
	leas	82,s
	puls	u
	rts
main:
	pshs	u
	leas	-82,s
	leau	,s
	ldd	#0
	std	4,u
	ldd	4,u
	std	0,u
	ldd	0,u
	std	4,u
	ldd	4,u
	lbeq	__Loptimise_6
	ldd	#1
	std	4,u
	jmp	__Loptimise_7
__Loptimise_6:
	ldd	#0
	std	4,u
__Loptimise_7:
	ldd	4,u
	lbeq	__Loptimise_5
	ldd	#0
	std	4,u
	ldd	4,u
	jsr	__printint
__Loptimise_5:
	ldd	#4
	std	4,u
	ldx	#thing
	stx	8,u
	ldx	8,u
	ldd	4,u
	std	,x
	ldx	#thing
	stx	4,u
	ldx	4,u
	ldd	,x
	std	4,u
	ldd	4,u
	pshs	d
	jsr	twice
	ldd	__ret
	std	4,u
	leas	2,s
	ldx	#thing
	stx	8,u
	ldd	8,u
	addd	#2
	std	8,u
	ldx	8,u
	ldd	4,u
	std	,x
	ldx	#thing
	stx	4,u
	ldd	4,u
	addd	#2
	std	4,u
	ldx	4,u
	ldd	,x
	std	4,u
	ldd	4,u
	jsr	__printint
__Loptimise_4:
	leas	82,s
	puls	u
	rts
