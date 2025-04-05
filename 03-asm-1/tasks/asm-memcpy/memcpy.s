  .intel_syntax noprefix

  .text
  .global my_memcpy

my_memcpy:
    push rbp
    mov rbp, rsp

    // обнуляем счетчик сделанных итераций
    xor ecx, ecx
    // копируем src в регистр r8
    mov r8, rdi
    // копируем dest в регистр r9
    mov r9, rsi
    // rsi = count
    mov rsi, rdx

    // сохроняем rbx
    push rbx
    // выравнивание по 16 байт
    push rbx

    // деление
    mov eax, esi
    mov ebx, 8
    xor edx, edx
    idiv ebx
    mov esi, eax
    // esi = count / 8  edx = count % 8

    cmp ecx, esi

    // case: count < 8
    jz after_loop

loop:
    mov rax, [r9 + rcx * 8]
    mov [r8 + rcx * 8], rax
    add rcx, 1
    cmp rcx, rsi
    jnz loop

after_loop:
    // сохроняем rcx
    mov r10, rcx

    // умножаем edx на 8
    mov eax, edx
    mov rbx, 8
    imul rbx
    mov ecx, eax
    // edx *= 8

    mov rax, [r9 + r10 * 8]

    // битовая маска для dest, r11
    xor r11, r11
    add r11, 1
    shl r11, cl
    sub r11, 1

    // битовая маска для scr, rbx
    mov rbx, 0xffffffffffffffff
    sub rbx, r11
    
    and rbx, [r8 + r10 * 8]
    and r11, rax

    xor rax, rax
    add rax, r11
    add rax, rbx

    mov [r8 + r10 * 8], rax

end:
    pop rbx
    pop rbx

    mov rax, r8

    mov rsp, rbp
    pop rbp
    ret