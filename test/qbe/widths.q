export function $no_result() {
@L2
  %frame =l alloc8 8
  %r0 =l copy 1
  %t1 =w copy %r0
  storeb %t1, %frame
@L1
  ret
}
export function l $frame_size() {
@L4
  %frame =l alloc8 8
  %r0 =l copy 1
  %t1 =w copy %r0
  storeb %t1, %frame
  %r0 =l copy 2
  %t2 =l add %frame, 1
  %t3 =w copy %r0
  storeb %t3, %t2
  %r0 =l copy 3
  %t4 =l add %frame, 2
  %t5 =w copy %r0
  storeb %t5, %t4
  %t6 =w loadub %frame
  %r2 =l extuw %t6
  %r3 =l copy 1
  %t7 =w ceql %r2, %r3
  %r2 =l extuw %t7
  %t8 =w ceql %r2, 0
  jnz %t8, @L7, @L9
@L9
  %t9 =l add %frame, 1
  %t10 =w loadub %t9
  %r2 =l extuw %t10
  %r3 =l copy 2
  %t11 =w ceql %r2, %r3
  %r2 =l extuw %t11
  %t12 =w ceql %r2, 0
  jnz %t12, @L7, @L10
@L10
  %r2 =l copy 1
  %r1 =l copy %r2
  jmp @L8
@L7
  %r2 =l copy 0
  %r1 =l copy %r2
@L8
  %t13 =w ceql %r1, 0
  jnz %t13, @L5, @L11
@L11
  %t14 =l add %frame, 2
  %t15 =w loadub %t14
  %r1 =l extuw %t15
  %r2 =l copy 3
  %t16 =w ceql %r1, %r2
  %r1 =l extuw %t16
  %t17 =w ceql %r1, 0
  jnz %t17, @L5, @L12
@L12
  %r1 =l copy 1
  %r0 =l copy %r1
  jmp @L6
@L5
  %r1 =l copy 0
  %r0 =l copy %r1
@L6
  ret %r0
@L3
  ret 0
}
export function l $narrow_widths() {
@L14
  %frame =l alloc8 16
  %r0 =l copy 255
  %t1 =w copy %r0
  storeb %t1, %frame
  %r0 =l copy -2
  %t2 =l add %frame, 2
  %t3 =w copy %r0
  storeh %t3, %t2
  %r0 =l copy -3
  %t4 =l add %frame, 4
  %t5 =w copy %r0
  storew %t5, %t4
  %r0 =l copy -4
  %t6 =l add %frame, 8
  storel %r0, %t6
  %t7 =w loadub %frame
  %r3 =l extuw %t7
  %t8 =l copy %r0
  %r0 =l copy 255
  %t9 =w ceql %r3, %r0
  %r3 =l extuw %t9
  %r0 =l copy %t8
  %t10 =w ceql %r3, 0
  jnz %t10, @L19, @L21
@L21
  %t11 =l add %frame, 2
  %t12 =w loadsh %t11
  %r3 =l extsw %t12
  %t13 =l copy %r1
  %r1 =l copy -2
  %t14 =w ceql %r3, %r1
  %r3 =l extuw %t14
  %r1 =l copy %t13
  %t15 =w ceql %r3, 0
  jnz %t15, @L19, @L22
@L22
  %r3 =l copy 1
  %r2 =l copy %r3
  jmp @L20
@L19
  %r3 =l copy 0
  %r2 =l copy %r3
@L20
  %t16 =w ceql %r2, 0
  jnz %t16, @L17, @L23
@L23
  %t17 =l add %frame, 4
  %r2 =l loadsw %t17
  %r3 =l copy -3
  %t18 =w ceql %r2, %r3
  %r2 =l extuw %t18
  %t19 =w ceql %r2, 0
  jnz %t19, @L17, @L24
@L24
  %r2 =l copy 1
  %r1 =l copy %r2
  jmp @L18
@L17
  %r2 =l copy 0
  %r1 =l copy %r2
@L18
  %t20 =w ceql %r1, 0
  jnz %t20, @L15, @L25
@L25
  %t21 =l add %frame, 8
  %r1 =l loadl %t21
  %r2 =l copy -4
  %t22 =w ceql %r1, %r2
  %r1 =l extuw %t22
  %t23 =w ceql %r1, 0
  jnz %t23, @L15, @L26
@L26
  %r1 =l copy 1
  %r0 =l copy %r1
  jmp @L16
@L15
  %r1 =l copy 0
  %r0 =l copy %r1
@L16
  ret %r0
@L13
  ret 0
}
export function l $array_widths() {
@L28
  %frame =l alloc8 32
  %r0 =l copy 0
  %r1 =l copy %frame
  %r2 =l copy 0
  %r1 =l add %r1, %r2
  %t1 =w copy %r0
  storeb %t1, %r1
  %r0 =l copy 0
  %r1 =l copy %frame
  %r2 =l copy 1
  %r1 =l add %r1, %r2
  %t2 =w copy %r0
  storeb %t2, %r1
  %r0 =l copy 0
  %r1 =l copy %frame
  %r1 =l add %r1, 2
  %r2 =l copy 0
  %r2 =l mul %r2, 2
  %r1 =l add %r1, %r2
  %t3 =w copy %r0
  storeh %t3, %r1
  %r0 =l copy 0
  %r1 =l copy %frame
  %r1 =l add %r1, 2
  %r2 =l copy 1
  %r2 =l mul %r2, 2
  %r1 =l add %r1, %r2
  %t4 =w copy %r0
  storeh %t4, %r1
  %r0 =l copy 0
  %r1 =l copy %frame
  %r1 =l add %r1, 8
  %r2 =l copy 0
  %r2 =l mul %r2, 4
  %r1 =l add %r1, %r2
  %t5 =w copy %r0
  storew %t5, %r1
  %r0 =l copy 0
  %r1 =l copy %frame
  %r1 =l add %r1, 8
  %r2 =l copy 1
  %r2 =l mul %r2, 4
  %r1 =l add %r1, %r2
  %t6 =w copy %r0
  storew %t6, %r1
  %r0 =l copy 0
  %r1 =l copy %frame
  %r1 =l add %r1, 16
  %r2 =l copy 0
  %r2 =l mul %r2, 8
  %r1 =l add %r1, %r2
  storel %r0, %r1
  %r0 =l copy 0
  %r1 =l copy %frame
  %r1 =l add %r1, 16
  %r2 =l copy 1
  %r2 =l mul %r2, 8
  %r1 =l add %r1, %r2
  storel %r0, %r1
  %r0 =l copy 1
  %r1 =l copy %frame
  %r2 =l copy 0
  %r1 =l add %r1, %r2
  %t7 =w copy %r0
  storeb %t7, %r1
  %r0 =l copy -2
  %r1 =l copy %frame
  %r1 =l add %r1, 2
  %r2 =l copy 0
  %r2 =l mul %r2, 2
  %r1 =l add %r1, %r2
  %t8 =w copy %r0
  storeh %t8, %r1
  %r0 =l copy -3
  %r1 =l copy %frame
  %r1 =l add %r1, 8
  %r2 =l copy 0
  %r2 =l mul %r2, 4
  %r1 =l add %r1, %r2
  %t9 =w copy %r0
  storew %t9, %r1
  %r0 =l copy -4
  %r1 =l copy %frame
  %r1 =l add %r1, 16
  %r2 =l copy 0
  %r2 =l mul %r2, 8
  %r1 =l add %r1, %r2
  storel %r0, %r1
  %r0 =l copy 10
  %r1 =l copy %frame
  %r2 =l copy 1
  %r1 =l add %r1, %r2
  %t10 =w copy %r0
  storeb %t10, %r1
  %r0 =l copy -20
  %r1 =l copy %frame
  %r1 =l add %r1, 2
  %r2 =l copy 1
  %r2 =l mul %r2, 2
  %r1 =l add %r1, %r2
  %t11 =w copy %r0
  storeh %t11, %r1
  %r0 =l copy -30
  %r1 =l copy %frame
  %r1 =l add %r1, 8
  %r2 =l copy 1
  %r2 =l mul %r2, 4
  %r1 =l add %r1, %r2
  %t12 =w copy %r0
  storew %t12, %r1
  %r0 =l copy -40
  %r1 =l copy %frame
  %r1 =l add %r1, 16
  %r2 =l copy 1
  %r2 =l mul %r2, 8
  %r1 =l add %r1, %r2
  storel %r0, %r1
  %r0 =l copy %frame
  %r0 =l add %r0, 2
  %r1 =l copy 1
  %r1 =l mul %r1, 2
  %r0 =l add %r0, %r1
  %t13 =w loadsh %r0
  %r0 =l extsw %t13
  %r1 =l copy -21
  %t14 =w cnel %r0, %r1
  jnz %t14, @L29, @L30
@L30
  %r0 =l copy 0
  ret %r0
@L29
  %t15 =l copy %r0
  %t16 =l copy %r1
  %t17 =l copy %r2
  %t18 =l copy %r3
  %r3 =l copy %frame
  %t19 =l copy %r0
  %r0 =l copy 0
  %r3 =l add %r3, %r0
  %r0 =l copy %t19
  %t20 =w loadub %r3
  %r3 =l extuw %t20
  %t21 =l copy %r1
  %r1 =l copy 1
  %t22 =w ceql %r3, %r1
  %r3 =l extuw %t22
  %r1 =l copy %t21
  %t23 =w ceql %r3, 0
  jnz %t23, @L43, @L45
@L45
  %r3 =l copy %t18
  %t24 =l copy %r2
  %r2 =l copy %frame
  %r2 =l add %r2, 2
  %t25 =l copy %r3
  %r3 =l copy 0
  %r3 =l mul %r3, 2
  %r2 =l add %r2, %r3
  %r3 =l copy %t25
  %t26 =w loadsh %r2
  %r2 =l extsw %t26
  %t27 =l copy %r0
  %r0 =l copy -2
  %t28 =w ceql %r2, %r0
  %r2 =l extuw %t28
  %r0 =l copy %t27
  %t29 =w ceql %r2, 0
  jnz %t29, @L43, @L46
@L46
  %r2 =l copy %t24
  %t30 =l copy %r1
  %r1 =l copy 1
  %r2 =l copy %r1
  %r1 =l copy %t30
  jmp @L44
@L43
  %t31 =l copy %r2
  %r2 =l copy 0
  %r2 =l copy %r2
  %r2 =l copy %t31
@L44
  %t32 =w ceql %r2, 0
  jnz %t32, @L41, @L47
@L47
  %r2 =l copy %t17
  %t33 =l copy %r3
  %r3 =l copy %frame
  %r3 =l add %r3, 8
  %t34 =l copy %r0
  %r0 =l copy 0
  %r0 =l mul %r0, 4
  %r3 =l add %r3, %r0
  %r0 =l copy %t34
  %r3 =l loadsw %r3
  %t35 =l copy %r1
  %r1 =l copy -3
  %t36 =w ceql %r3, %r1
  %r3 =l extuw %t36
  %r1 =l copy %t35
  %t37 =w ceql %r3, 0
  jnz %t37, @L41, @L48
@L48
  %r3 =l copy %t33
  %t38 =l copy %r2
  %r2 =l copy 1
  %r1 =l copy %r2
  %r2 =l copy %t38
  jmp @L42
@L41
  %t39 =l copy %r3
  %r3 =l copy 0
  %r1 =l copy %r3
  %r3 =l copy %t39
@L42
  %t40 =w ceql %r1, 0
  jnz %t40, @L39, @L49
@L49
  %r1 =l copy %t16
  %t41 =l copy %r0
  %r0 =l copy %frame
  %r0 =l add %r0, 16
  %t42 =l copy %r1
  %r1 =l copy 0
  %r1 =l mul %r1, 8
  %r0 =l add %r0, %r1
  %r1 =l copy %t42
  %r0 =l loadl %r0
  %t43 =l copy %r2
  %r2 =l copy -4
  %t44 =w ceql %r0, %r2
  %r0 =l extuw %t44
  %r2 =l copy %t43
  %t45 =w ceql %r0, 0
  jnz %t45, @L39, @L50
@L50
  %r0 =l copy %t41
  %t46 =l copy %r3
  %r3 =l copy 1
  %r0 =l copy %r3
  %r3 =l copy %t46
  jmp @L40
@L39
  %t47 =l copy %r0
  %r0 =l copy 0
  %r0 =l copy %r0
  %r0 =l copy %t47
@L40
  %t48 =w ceql %r0, 0
  jnz %t48, @L37, @L51
@L51
  %r0 =l copy %t15
  %t49 =l copy %r1
  %r1 =l copy %frame
  %t50 =l copy %r2
  %r2 =l copy 1
  %r1 =l add %r1, %r2
  %r2 =l copy %t50
  %t51 =w loadub %r1
  %r1 =l extuw %t51
  %t52 =l copy %r3
  %r3 =l copy 10
  %t53 =w ceql %r1, %r3
  %r1 =l extuw %t53
  %r3 =l copy %t52
  %t54 =w ceql %r1, 0
  jnz %t54, @L37, @L52
@L52
  %r1 =l copy %t49
  %t55 =l copy %r0
  %r0 =l copy 1
  %r3 =l copy %r0
  %r0 =l copy %t55
  jmp @L38
@L37
  %t56 =l copy %r1
  %r1 =l copy 0
  %r3 =l copy %r1
  %r1 =l copy %t56
@L38
  %t57 =w ceql %r3, 0
  jnz %t57, @L35, @L53
@L53
  %r3 =l copy %frame
  %r3 =l add %r3, 2
  %t58 =l copy %r2
  %r2 =l copy 1
  %r2 =l mul %r2, 2
  %r3 =l add %r3, %r2
  %r2 =l copy %t58
  %t59 =w loadsh %r3
  %r3 =l extsw %t59
  %t60 =l copy %r3
  %r3 =l copy -20
  %t61 =l copy %r3
  %r3 =l copy %t60
  %t62 =w ceql %r3, %t61
  %r3 =l extuw %t62
  %t63 =w ceql %r3, 0
  jnz %t63, @L35, @L54
@L54
  %r3 =l copy 1
  %r2 =l copy %r3
  jmp @L36
@L35
  %r3 =l copy 0
  %r2 =l copy %r3
@L36
  %t64 =w ceql %r2, 0
  jnz %t64, @L33, @L55
@L55
  %r2 =l copy %frame
  %r2 =l add %r2, 8
  %r3 =l copy 1
  %r3 =l mul %r3, 4
  %r2 =l add %r2, %r3
  %r2 =l loadsw %r2
  %r3 =l copy -30
  %t65 =w ceql %r2, %r3
  %r2 =l extuw %t65
  %t66 =w ceql %r2, 0
  jnz %t66, @L33, @L56
@L56
  %r2 =l copy 1
  %r1 =l copy %r2
  jmp @L34
@L33
  %r2 =l copy 0
  %r1 =l copy %r2
@L34
  %t67 =w ceql %r1, 0
  jnz %t67, @L31, @L57
@L57
  %r1 =l copy %frame
  %r1 =l add %r1, 16
  %r2 =l copy 1
  %r2 =l mul %r2, 8
  %r1 =l add %r1, %r2
  %r1 =l loadl %r1
  %r2 =l copy -40
  %t68 =w ceql %r1, %r2
  %r1 =l extuw %t68
  %t69 =w ceql %r1, 0
  jnz %t69, @L31, @L58
@L58
  %r1 =l copy 1
  %r0 =l copy %r1
  jmp @L32
@L31
  %r1 =l copy 0
  %r0 =l copy %r1
@L32
  ret %r0
@L27
  ret 0
}
export function l $narrow_argument(l %arg0, l %arg1) {
@L60
  %frame =l alloc8 8
  %t1 =w copy %arg0
  storeb %t1, %frame
  %t2 =l add %frame, 2
  %t3 =w copy %arg1
  storeh %t3, %t2
  %t4 =l add %frame, 2
  %t5 =w loadsh %t4
  %r0 =l extsw %t5
  %r1 =l copy -100
  %t6 =w ceql %r0, %r1
  jnz %t6, @L61, @L62
@L62
  %r0 =l copy 0
  ret %r0
@L61
  %t7 =w loadub %frame
  %r0 =l extuw %t7
  ret %r0
@L59
  ret 0
}
export function l $main() {
@L64
  call $no_result()
  %r0 =l call $frame_size()
  %t1 =w ceql %r0, 0
  %r0 =l extuw %t1
  %t2 =w cnel %r0, 0
  %r0 =l extuw %t2
  %t3 =w ceql %r0, 0
  jnz %t3, @L65, @L66
@L66
  %r0 =l copy 1
  ret %r0
@L65
  %r0 =l call $narrow_widths()
  %t4 =w ceql %r0, 0
  %r0 =l extuw %t4
  %t5 =w cnel %r0, 0
  %r0 =l extuw %t5
  %t6 =w ceql %r0, 0
  jnz %t6, @L67, @L68
@L68
  %r0 =l copy 2
  ret %r0
@L67
  %r0 =l call $array_widths()
  %t7 =w ceql %r0, 0
  %r0 =l extuw %t7
  %t8 =w cnel %r0, 0
  %r0 =l extuw %t8
  %t9 =w ceql %r0, 0
  jnz %t9, @L69, @L70
@L70
  %r0 =l copy 3
  ret %r0
@L69
  %r0 =l copy 263
  %r0 =l and %r0, 255
  %r1 =l copy 7
  %t10 =w ceql %r0, %r1
  jnz %t10, @L71, @L72
@L72
  %r0 =l copy 4
  ret %r0
@L71
  %r0 =l copy 65436
  %r0 =l extsh %r0
  %r1 =l copy -100
  %t11 =w ceql %r0, %r1
  jnz %t11, @L73, @L74
@L74
  %r0 =l copy 5
  ret %r0
@L73
  %r0 =l copy 65436
  %r0 =l extsh %r0
  %t12 =l copy %r0
  %r0 =l copy 263
  %r0 =l and %r0, 255
  %t13 =l copy %r0
  %r0 =l call $narrow_argument(l %t13, l %t12)
  %r1 =l copy 7
  %t14 =w ceql %r0, %r1
  jnz %t14, @L75, @L76
@L76
  %r0 =l copy 6
  ret %r0
@L75
  %r0 =l copy 0
  ret %r0
@L63
  ret 0
}
