	.text
	.data
	.globl	_letter
_letter:
	.byte	0
	.text
	.data
	.globl	_number
	.balign	8
_number:
	.quad	0
	.text
	.text
	.globl	_twice
_twice:
	sub	sp, sp, #192
	stp	x29, x30, [sp]
	mov	x29, sp
	stp	x19, x20, [sp, #16]
	stp	x21, x22, [sp, #32]
	str	w0, [x29, #48]
	ldrsw	x19, [x29, #48]
	movz	x20, #2
	mul	x19, x19, x20
	mov	w0, w19
L1:
	ldp	x19, x20, [sp, #16]
	ldp	x21, x22, [sp, #32]
	ldp	x29, x30, [sp]
	add	sp, sp, #192
	ret
	.text
	.globl	_main
_main:
	sub	sp, sp, #192
	stp	x29, x30, [sp]
	mov	x29, sp
	stp	x19, x20, [sp, #16]
	stp	x21, x22, [sp, #32]
	movz	x19, #3
	adrp	x17, _number@PAGE
	add	x17, x17, _number@PAGEOFF
	str	x19, [x17]
	adrp	x17, _number@PAGE
	add	x17, x17, _number@PAGEOFF
	ldr	x19, [x17]
	movz	x20, #1
	add	x19, x19, x20
	str	x19, [x29, #56]
	adrp	x17, _number@PAGE
	add	x17, x17, _number@PAGEOFF
	str	x19, [x17]
	movz	x19, #97
	strb	w19, [x29, #48]
	sub	sp, sp, #16
	str	x19, [sp]
	ldr	x0, [sp, #0]
	add	sp, sp, #16
	bl	_printchar
	mov	x19, x0
	ldrb	w19, [x29, #48]
	adrp	x17, _letter@PAGE
	add	x17, x17, _letter@PAGEOFF
	strb	w19, [x17]
	adrp	x17, _letter@PAGE
	add	x17, x17, _letter@PAGEOFF
	ldrb	w19, [x17]
	sub	sp, sp, #16
	str	x19, [sp]
	ldr	x0, [sp, #0]
	add	sp, sp, #16
	bl	_twice
	sxtw	x19, w0
	mov	x0, x19
	bl	_printint
	adrp	x17, _number@PAGE
	add	x17, x17, _number@PAGEOFF
	ldr	x19, [x17]
	sxtw	x19, w19
	mov	x0, x19
	bl	_printint
L2:
	ldp	x19, x20, [sp, #16]
	ldp	x21, x22, [sp, #32]
	ldp	x29, x30, [sp]
	add	sp, sp, #192
	ret
