[BITS 32]
[ORG 0h]


[SECTION .text]

mov ebx, str
mov eax, 0x0
int 0x80

jmp $

[SECTION .data]

str:	db	"Hello, I am a user program!", 0xa
