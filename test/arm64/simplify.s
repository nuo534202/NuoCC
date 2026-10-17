	.text
	.data
	.globl	_value
	.balign	4
_value:
	.long	0
	.text
	.data
	.globl	_count
	.balign	4
_count:
	.long	0
	.text
	.text
	.globl	_bump
_bump:
	sub	sp, sp, #176
	stp	x29, x30, [sp]
	mov	x29, sp
	stp	x19, x20, [sp, #16]
	stp	x21, x22, [sp, #32]
	adrp	x17, _count@PAGE
	add	x17, x17, _count@PAGEOFF
	ldrsw	x19, [x17]
	movz	x20, #1
	add	x19, x19, x20
	adrp	x17, _count@PAGE
	add	x17, x17, _count@PAGEOFF
	str	w19, [x17]
	movz	x19, #3
	mov	w0, w19
L1:
	ldp	x19, x20, [sp, #16]
	ldp	x21, x22, [sp, #32]
	ldp	x29, x30, [sp]
	add	sp, sp, #176
	ret
	.text
	.globl	_main
_main:
	sub	sp, sp, #176
	stp	x29, x30, [sp]
	mov	x29, sp
	stp	x19, x20, [sp, #16]
	stp	x21, x22, [sp, #32]
	movz	x19, #5
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	str	w19, [x17]
	movz	x19, #0
	adrp	x17, _count@PAGE
	add	x17, x17, _count@PAGEOFF
	str	w19, [x17]
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	movz	x20, #1
	lsl	x19, x19, x20
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	movz	x20, #3
	lsl	x19, x19, x20
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	movz	x20, #1
	lsl	x19, x19, x20
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	movz	x20, #2
	sdiv	x19, x19, x20
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	movz	x20, #2
	sdiv	x21, x19, x20
	msub	x19, x21, x20, x19
	mov	x0, x19
	bl	_printint
	movz	x19, #0
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x20, [x17]
	sub	x19, x19, x20
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	movz	x20, #0
	mul	x19, x19, x20
	mov	x0, x19
	bl	_printint
	adrp	x17, _value@PAGE
	add	x17, x17, _value@PAGEOFF
	ldrsw	x19, [x17]
	movz	x20, #0
	and	x19, x19, x20
	mov	x0, x19
	bl	_printint
	bl	_bump
	sxtw	x19, w0
	movz	x20, #0
	mul	x19, x19, x20
	mov	x0, x19
	bl	_printint
	bl	_bump
	sxtw	x19, w0
	mov	x0, x19
	bl	_printint
	adrp	x17, _count@PAGE
	add	x17, x17, _count@PAGEOFF
	ldrsw	x19, [x17]
	mov	x0, x19
	bl	_printint
L2:
	ldp	x19, x20, [sp, #16]
	ldp	x21, x22, [sp, #32]
	ldp	x29, x30, [sp]
	add	sp, sp, #176
	ret
