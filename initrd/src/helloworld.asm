[BITS 32]
[ORG 0h]


[SECTION .text]

mov eax, 0x0
mov ebx, str
int 0x80

jmp $

[SECTION .data]

str:	db	"Hello user world!", 0xa
