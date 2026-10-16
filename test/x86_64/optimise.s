	.text
	.data
	.globl	letter
letter:
	.byte	0
	.text
	.data
	.globl	number
	.balign	8
number:
	.quad	0
	.text
	.text
	.globl	twice
	.type	twice, @function
twice:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$144, %rsp
	movl	%edi, -4(%rbp)
	movslq	-4(%rbp), %r8
	movq	$2, %r9
	imulq	%r9, %r8
	movl	%r8d, %eax
L1:
	movq	%rbp, %rsp
	popq %rbp
	ret
	.text
	.globl	main
	.type	main, @function
main:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$144, %rsp
	movq	$3, %r8
	movq	%r8, number(%rip)
	movq	$1, %r9
	addq	%r9, %r8
	movq	%r8, -16(%rbp)
	movq	%r8, number(%rip)
	movq	$97, %r8
	movb	%r8b, -1(%rbp)
	movzbq	-1(%rbp), %r8
	subq	$16, %rsp
	movq	%r8, (%rsp)
	movq	0(%rsp), %rdi
	call	printchar
	addq	$16, %rsp
	movq	%rax, %r8
	movzbq	-1(%rbp), %r8
	movb	%r8b, letter(%rip)
	movzbq	letter(%rip), %r8
	subq	$16, %rsp
	movq	%r8, (%rsp)
	movq	0(%rsp), %rdi
	call	twice
	addq	$16, %rsp
	movslq	%eax, %r8
	movq	%r8, %rdi
	call	printint

	movq	number(%rip), %r8
	movslq	%r8d, %r8
	movq	%r8, %rdi
	call	printint

L2:
	movq	%rbp, %rsp
	popq %rbp
	ret
