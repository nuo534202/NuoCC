thing:
	rmb	4
twice:
	pshs	u
	leas	-82,s
	leau	,s
	ldd	86,u
	std	0,u
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
	leas	-80,s
	leau	,s
	ldd	#4
	std	2,u
	ldx	#thing
	stx	6,u
	ldd	2,u
	std	,x
	ldx	#thing
	stx	2,u
	ldd	,x
	std	2,u
	pshs	d
	jsr	twice
	ldd	__ret
	std	2,u
	leas	2,s
	ldx	#thing
	stx	6,u
	ldd	6,u
	addd	#2
	std	6,u
	ldx	6,u
	ldd	2,u
	std	,x
	ldx	#thing
	stx	2,u
	ldd	2,u
	addd	#2
	std	2,u
	ldx	2,u
	ldd	,x
	std	2,u
	jsr	__printint
__Loptimise_4:
	leas	80,s
	puls	u
	rts
