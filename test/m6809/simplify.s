value:
	.word	0
count:
	.word	0
bump:
	pshs	u
	leas	-80,s
	leau	,s
	ldd	count
	std	2,u
	ldd	#1
	std	6,u
	ldd	2,u
	addd	6,u
	std	2,u
	std	count
	ldd	#3
	std	2,u
	std	__ret
__Lsimplify_1:
	leas	80,s
	puls	u
	rts
main:
	pshs	u
	leas	-80,s
	leau	,s
	ldd	#5
	std	2,u
	std	value
	ldd	#0
	std	2,u
	std	count
	ldd	value
	std	2,u
	ldd	#1
	std	6,u
	ldb	7,u
	lbeq	__Lsimplify_4
__Lsimplify_3:
	lsl	3,u
	rol	2,u
	decb
	lbne	__Lsimplify_3
__Lsimplify_4:
	ldd	2,u
	jsr	__printint
	ldd	value
	std	2,u
	ldd	#3
	std	6,u
	ldb	7,u
	lbeq	__Lsimplify_6
__Lsimplify_5:
	lsl	3,u
	rol	2,u
	decb
	lbne	__Lsimplify_5
__Lsimplify_6:
	ldd	2,u
	jsr	__printint
	ldd	value
	std	2,u
	ldd	#1
	std	6,u
	ldb	7,u
	lbeq	__Lsimplify_8
__Lsimplify_7:
	lsl	3,u
	rol	2,u
	decb
	lbne	__Lsimplify_7
__Lsimplify_8:
	ldd	2,u
	jsr	__printint
	ldd	value
	std	2,u
	jsr	__printint
	ldd	value
	std	2,u
	jsr	__printint
	ldd	value
	std	2,u
	jsr	__printint
	ldd	value
	std	2,u
	jsr	__printint
	ldd	value
	std	2,u
	jsr	__printint
	ldd	value
	std	2,u
	jsr	__printint
	ldd	value
	std	2,u
	jsr	__printint
	ldd	value
	std	2,u
	jsr	__printint
	ldd	value
	std	2,u
	jsr	__printint
	ldd	value
	std	2,u
	ldd	#2
	std	6,u
	leax	2,u
	leay	6,u
	jsr	__div16
	ldd	2,u
	jsr	__printint
	ldd	value
	std	2,u
	ldd	#2
	std	6,u
	leax	2,u
	leay	6,u
	jsr	__rem16
	ldd	2,u
	jsr	__printint
	ldd	#0
	std	2,u
	ldd	value
	std	6,u
	ldd	2,u
	subd	6,u
	std	2,u
	jsr	__printint
	ldd	value
	std	2,u
	ldd	#0
	std	6,u
	leax	2,u
	leay	6,u
	jsr	__mul16
	ldd	2,u
	jsr	__printint
	ldd	value
	std	2,u
	ldd	#0
	std	6,u
	ldd	2,u
	anda	6,u
	andb	7,u
	std	2,u
	jsr	__printint
	jsr	bump
	ldd	__ret
	std	2,u
	ldd	#0
	std	6,u
	leax	2,u
	leay	6,u
	jsr	__mul16
	ldd	2,u
	jsr	__printint
	jsr	bump
	ldd	__ret
	std	2,u
	jsr	__printint
	ldd	count
	std	2,u
	jsr	__printint
__Lsimplify_2:
	leas	80,s
	puls	u
	rts
