main:
	pshs	u
	leas	-88,s
	leau	,s
	ldd	#0
	std	8,u
	ldd	#32768
	std	10,u
	ldd	10,u
	std	0,u
	ldd	0,u
	std	10,u
	ldd	#0
	std	8,u
	ldd	8,u
	std	4,u
	ldd	10,u
	std	6,u
	ldd	4,u
	std	8,u
	ldd	6,u
	std	10,u
	ldd	#0
	std	12,u
	ldd	#32768
	std	14,u
	ldd	8,u
	cmpd	12,u
	lbne	__Lpointer_widen_2
	ldd	10,u
	cmpd	14,u
	lbne	__Lpointer_widen_2
	ldd	#1
	std	10,u
	ldd	10,u
	jsr	__printint
__Lpointer_widen_2:
	ldd	#0
	std	10,u
	ldd	10,u
	std	__ret
__Lpointer_widen_1:
	leas	88,s
	puls	u
	rts
