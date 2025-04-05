.intel_syntax noprefix

.text
.global add_scanf

add_scanf:
  sub rsp, 24
  mov rdi, offset scanf_format_string
  lea rsi, [rsp]
  lea rdx, [rsp + 8]
  xor rax, rax
  call scanf

  mov rax, [rsp]
  add rax, [rsp + 8]

  add rsp, 24
  ret

.section .rodata

scanf_format_string:
  .string "%lld %lld"