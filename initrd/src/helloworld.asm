[BITS 32]
[ORG 0h]


[SECTION .text]

mov eax, 0x1
int 0x80

mov ebx, str
mov eax, 0x0
int 0x80



jmp $

[SECTION .data]

str:	db	"Hello, I am a forking user program!", 0xa
