.intel_syntax noprefix

.text
.global dot_product

dot_product:
    push rbp
    mov rbp, rsp

    mov r8, rdi 
    mov r9, rsi
    mov r10, rdx
    // r8 = N, r9 = A, r10 = b

    // сохраняем rbx
    push rbx
    // выравнивание по 16 байт
    push rbx

    // деление N на 2
    mov rax, r8
    mov rbx, 2
    xor rdx, rdx
    idiv rbx
    // rax = N / 2  rdx = N % 2

    mov rdx, r8
    mov r8, rax

    // остаток от деление на 8 числа N
    mov cl, 3
    // битовая маска
    xor r11, r11
    add r11, 1
    shl r11, cl
    and rdx, r11
    // rdx = N % 8

    // подготовка к циклу
    xor rcx, rcx
    vzeroall

    cmp rdx, 0

    jnz case1

    // ymm3 == ans
loop:
    vmovups ymm0, [r9 + rcx * 8]
    vmovups ymm1, [r10 + rcx * 8]

    // умножение
    vmulps ymm0, ymm0, ymm1

    // сложение
    vhaddps ymm0, ymm0, ymm0
    vhaddps ymm0, ymm0, ymm0
    vperm2f128 ymm1, ymm0, ymm0, 1

    // скалярное произведение 8 элементов
    vaddps ymm0, ymm0, ymm1
    vaddps ymm3, ymm0, ymm3

    // ymm3 = [t][t][t][t][t][t][t][ans]
    // t - trash

    // итерация
    add rcx, 4
    cmp rcx, r8
    jl loop

    jmp end

case1:
    add r8, 4
    jmp loop

end:
    vmovups ymm0, ymm3

    pop rbx
    pop rbx

    mov rsp, rbp
    pop rbp
    ret