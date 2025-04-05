.text
.global longest_inc_subseq

longest_inc_subseq:
    mov x3, 0
    mov x6, 1

outer_loop:
    cmp x3, x2
    bge outer_loop_end
    str x6, [x1, x3, lsl 3]

    mov x4, 0

inner_loop:
    cmp x4, x3
    bge inner_loop_end

    ldr x7, [x0, x3, lsl 3]
    ldr x8, [x0, x4, lsl 3]
    cmp x8, x7
    bge skip_update

    ldr x9, [x1, x4, lsl 3]
    add x9, x9, 1
    ldr x10, [x1, x3, lsl 3]
    cmp x10, x9
    bge skip_update

    str x9, [x1, x3, lsl 3]

skip_update:
    add x4, x4, 1
    b inner_loop

inner_loop_end:
    add x3, x3, 1
    b outer_loop

outer_loop_end:
    mov x3, 0
    mov x0, 0

find_max:
    cmp x3, x2
    bge end

    ldr x4, [x1, x3, lsl 3]
    cmp x0, x4
    bge continue_max
    mov x0, x4

continue_max:
    add x3, x3, 1
    b find_max

end:
    ret
