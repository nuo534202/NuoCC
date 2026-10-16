; The runtime a 6809 program is built with: where it begins, the two
; routines which write out what a program prints, and the arithmetic the
; machine has no instruction for. A program is the runtime with the code
; the compiler wrote after it, so what is here is the other half of the
; program.
;
; The programs are run by the emulator described in test/m6809/README.md,
; which writes a character, writes a value out in decimal and leaves the
; program by watching the memory it is told to write: 0xFEFE is one
; character, 0xFEFC and 0xFEFD are the two halves of a value, and 0xFEFF
; leaves the program with the byte it is given. A program of a machine of
; its own would find these in its operating system instead.
;
; A value is held as the whole of it, big endian, in the bytes at the
; address it is given: two bytes for an int and four for a long. A routine
; which works out one value from two is given the address of the left one
; in X and the address of the right one in Y, and leaves its answer both
; in the left value and in D.

        ; The header the emulator reads from the first sixteen bytes of the
        ; program: what it is, where it is loaded and where it begins.
        .word   0x80A8          ; the mark of the format
        .byte   4               ; the 6809
        .byte   0               ; no 6309 instruction is used
        .byte   0               ; loaded at the start of memory
        .byte   0               ; no hints for the emulator
        .word   0               ; the size of the code
        .word   0               ; the size of the data
        .word   0               ; the size of the storage which starts empty
        .byte   16              ; the instructions begin sixteen bytes in
        .byte   0               ; where the storage ends
        .byte   0
        .byte   0               ; the direct page is not used

; Where the program begins: the stack the emulator leaves is put where the
; language wants it, the program is run, and the program is left when it
; is over.
entry:
        lds     #0xFDFF
        jsr     main
        clr     0xFEFF          ; leave saying nothing went wrong

; Write out the value a print statement is given, which the compiler
; leaves in D before it calls here.
__printint:
        sta     0xFEFC
        stb     0xFEFD
        rts

; Write out the value in D in decimal, which is what a program calling
; printint() asks for. The argument is on the stack above the return
; address, where the language leaves every argument.
printint:
        ldd     2,s
        bra     __printint

; Write out the character a program calling printchar() asks for, which is
; the low byte of the value on the stack.
printchar:
        ldb     3,s
        stb     0xFEFE
        rts

; ---------------------------------------------------------------------
; Multiplication
; ---------------------------------------------------------------------

; Multiply two 16-bit values, answering with the low sixteen bits of the
; product. Only the low bytes of the two crossings over are wanted, since
; the rest of them falls outside the answer.
__mul16:
        lda     0,x             ; the high half of the left value
        ldb     1,y             ; the low half of the right one
        mul
        stb     __mulh
        lda     1,x
        ldb     0,y
        mul
        addb    __mulh
        stb     __mulh
        lda     1,x             ; the two low halves land on the low half
        ldb     1,y
        mul
        adda    __mulh          ; and the crossings make up the high half
        std     0,x
        rts

; Multiply two 32-bit values, adding the left value into the answer once
; for every bit of the right one and moving the two of them along.
__mul32:
        pshs    x,y
        ldd     0,x
        std     __m32a
        ldd     2,x
        std     __m32a+2
        ldd     0,y
        std     __m32b
        ldd     2,y
        std     __m32b+2
        ldd     #0
        std     __m32r
        std     __m32r+2
        ldx     #32
__mul32_loop:
        ldd     __m32b+2
        lsra
        rorb                    ; the lowest bit of the right value
        bcc     __mul32_next
        ldd     __m32r+2        ; it is set, so the left value is added
        addd    __m32a+2
        std     __m32r+2
        ldd     __m32r
        adcb    __m32a+1
        adca    __m32a
        std     __m32r
__mul32_next:
        lsl     __m32a+3        ; the left value moves along
        rol     __m32a+2
        rol     __m32a+1
        rol     __m32a
        lsr     __m32b          ; and the right one moves down
        ror     __m32b+1
        ror     __m32b+2
        ror     __m32b+3
        leax    -1,x
        bne     __mul32_loop
        puls    x,y
        ldd     __m32r
        std     0,x
        ldd     __m32r+2
        std     2,x
        rts

; ---------------------------------------------------------------------
; Division
; ---------------------------------------------------------------------

; Work out the quotient and the remainder of two 16-bit values at once, so
; that both of the two routines below have only to pick one of them out.
; Both values are made positive to be divided, and the signs are put back
; afterwards: the quotient is negative when only one of the two values is,
; and the remainder has the sign of the left value.
__divmod16:
        ldd     0,y
        lbeq    __divzero
        std     __dv_b
        ldd     0,x
        std     __dv_a
        clr     __dv_qneg
        clr     __dv_rneg
        ldd     __dv_a
        bpl     __dv_apos
        nega
        negb
        sbca    #0
        std     __dv_a
        com     __dv_qneg
        com     __dv_rneg
__dv_apos:
        ldd     __dv_b
        bpl     __dv_bpos
        nega
        negb
        sbca    #0
        std     __dv_b
        com     __dv_qneg
__dv_bpos:
        ldd     #0
        std     __dv_q
        std     __dv_r
        ldy     #16
__dv_loop:
        ldd     __dv_a          ; the top bit of what is left of the
        lslb                    ; dividend comes down into the remainder
        rola
        rol     __dv_r+1
        rol     __dv_r
        std     __dv_a
        lsl     __dv_q+1        ; and the answer grows a bit
        rol     __dv_q
        ldd     __dv_r          ; does the divisor go into it?
        subd    __dv_b
        blo     __dv_next
        std     __dv_r          ; it does: keep what is left of it
        ldb     __dv_q+1
        orb     #1              ; and this bit of the answer is a one
        stb     __dv_q+1
__dv_next:
        leay    -1,y
        bne     __dv_loop
        ldd     __dv_q          ; put the sign back on the answer
        tst     __dv_qneg
        beq     __dv_qpos
        nega
        negb
        sbca    #0
        std     __dv_q
__dv_qpos:
        ldd     __dv_r
        tst     __dv_rneg
        beq     __dv_rpos
        nega
        negb
        sbca    #0
        std     __dv_r
__dv_rpos:
        rts

__divzero:
        ldb     #1              ; leaving with a byte which is not nothing
        stb     0xFEFF          ; stops the program, which is what the
        rts                     ; machine does with a division by zero

; Divide the left 16-bit value by the right one.
__div16:
        jsr     __divmod16
        ldd     __dv_q
        std     0,x
        rts

; What is left of the left 16-bit value once the right one has been taken
; from it as many whole times as it holds.
__rem16:
        jsr     __divmod16
        ldd     __dv_r
        std     0,x
        rts

; The same for two 32-bit values, one bit at a time.
__divmod32:
        pshs    x,y
        ldd     0,x
        std     __dv32_a
        ldd     2,x
        std     __dv32_a+2
        ldd     0,y
        std     __dv32_b
        ldd     2,y
        std     __dv32_b+2
        ldd     __dv32_b
        bne     __dv32_go
        ldd     __dv32_b+2
        lbeq    __divzero
__dv32_go:
        clr     __dv32_qneg
        clr     __dv32_rneg
        tst     __dv32_a
        bpl     __dv32_apos
        ldx     #__dv32_a
        jsr     __neg32
        com     __dv32_qneg
        com     __dv32_rneg
__dv32_apos:
        tst     __dv32_b
        bpl     __dv32_bpos
        ldx     #__dv32_b
        jsr     __neg32
        com     __dv32_qneg
__dv32_bpos:
        ldd     #0
        std     __dv32_q
        std     __dv32_q+2
        std     __dv32_r
        std     __dv32_r+2
        ldx     #32
__dv32_loop:
        lsl     __dv32_a+3      ; the top bit of what is left of the
        rol     __dv32_a+2      ; dividend comes down into the remainder
        rol     __dv32_a+1
        rol     __dv32_a
        rol     __dv32_r+3
        rol     __dv32_r+2
        rol     __dv32_r+1
        rol     __dv32_r
        lsl     __dv32_q+3      ; and the answer grows a bit
        rol     __dv32_q+2
        rol     __dv32_q+1
        rol     __dv32_q
        ldd     __dv32_r+2      ; does the divisor go into it? The low
        subd    __dv32_b+2      ; halves are taken away first, and what
        std     __dv32_t+2      ; the borrow leaves is kept aside
        ldd     __dv32_r
        sbcb    __dv32_b+1
        sbca    __dv32_b
        blo     __dv32_next
        std     __dv32_r        ; it does: keep what is left of it
        ldd     __dv32_t+2
        std     __dv32_r+2
        ldb     __dv32_q+3
        orb     #1              ; and this bit of the answer is a one
        stb     __dv32_q+3
__dv32_next:
        leax    -1,x
        bne     __dv32_loop
        tst     __dv32_qneg     ; put the signs back on the answers
        beq     __dv32_qpos
        ldx     #__dv32_q
        jsr     __neg32
__dv32_qpos:
        tst     __dv32_rneg
        beq     __dv32_rpos
        ldx     #__dv32_r
        jsr     __neg32
__dv32_rpos:
        puls    x,y
        rts

; The negative of the 32-bit value at the address in X: the whole of the
; value is complemented and one is added to it.
__neg32:
        com     0,x
        com     1,x
        com     2,x
        com     3,x
        ldd     2,x
        addd    #1
        std     2,x
        ldd     0,x
        adcb    #0
        adca    #0
        std     0,x
        rts

; Divide the left 32-bit value by the right one, and answer with what the
; division leaves behind. The work is the same for both of these, and the
; routine which does it leaves the two addresses it was given alone.
__div32:
        jsr     __divmod32
        ldd     __dv32_q
        std     0,x
        ldd     __dv32_q+2
        std     2,x
        rts

__rem32:
        jsr     __divmod32
        ldd     __dv32_r
        std     0,x
        ldd     __dv32_r+2
        std     2,x
        rts

; ---------------------------------------------------------------------
; The storage the routines above work in, which is zero to start with and
; is never read before it is written.
; ---------------------------------------------------------------------
__mulh:         .byte   0
__m32a:         .word   0
                .word   0
__m32b:         .word   0
                .word   0
__m32r:         .word   0
                .word   0
__dv_a:         .word   0
__dv_b:         .word   0
__dv_q:         .word   0
__dv_r:         .word   0
__dv_qneg:      .byte   0
__dv_rneg:      .byte   0
__dv32_a:       .word   0
                .word   0
__dv32_b:       .word   0
                .word   0
__dv32_q:       .word   0
                .word   0
__dv32_r:       .word   0
                .word   0
__dv32_t:       .word   0
                .word   0
__dv32_qneg:    .byte   0
__dv32_rneg:    .byte   0

; The value a function answers with, and the stride a value is moved along
; by when it is not known while the program is being compiled. The compiler
; writes these and reads them back.
__ret:          .word   0
                .word   0
__scale:        .word   0
                .word   0
