	.text
	.data
	.globl	value
	.balign	4
value:
	.long	0
	.text
	.data
	.globl	count
	.balign	4
count:
	.long	0
	.text
	.text
	.globl	bump
	.type	bump, @function
bump:
	pushq	%rbp
	movq	%rsp, %rbp
	subq	$128, %rsp
	movslq	count(%rip), %r8
	movq	$1, %r9
	addq	%r9, %r8
	movl	%r8d, count(%rip)
	movq	$3, %r8
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
	subq	$128, %rsp
	movq	$5, %r8
	movl	%r8d, value(%rip)
	movq	$0, %r8
	movl	%r8d, count(%rip)
	movslq	value(%rip), %r8
	movq	$1, %r9
	movb	%r9b, %cl
	shlq	%cl, %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	$3, %r9
	movb	%r9b, %cl
	shlq	%cl, %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	$1, %r9
	movb	%r9b, %cl
	shlq	%cl, %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	$2, %r9
	movq	%r8, %rax
	cqo
	idivq	%r9
	movq	%rax, %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	$2, %r9
	movq	%r8, %rax
	cqo
	idivq	%r9
	movq	%rdx, %r8
	movq	%r8, %rdi
	call	printint

	movq	$0, %r8
	movslq	value(%rip), %r9
	subq	%r9, %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	$0, %r9
	imulq	%r9, %r8
	movq	%r8, %rdi
	call	printint

	movslq	value(%rip), %r8
	movq	$0, %r9
	andq	%r9, %r8
	movq	%r8, %rdi
	call	printint

	call	bump
	movslq	%eax, %r8
	movq	$0, %r9
	imulq	%r9, %r8
	movq	%r8, %rdi
	call	printint

	call	bump
	movslq	%eax, %r8
	movq	%r8, %rdi
	call	printint

	movslq	count(%rip), %r8
	movq	%r8, %rdi
	call	printint

L2:
	movq	%rbp, %rsp
	popq %rbp
	ret
