section .text

global get_eip
global become_user

global switch_context

get_eip:
	pop eax
	jmp eax

switch_context:
	cli
	mov ecx, [esp+4]   ; eip
	mov eax, [esp+8]   ; physical address of current directory
	mov ebp, [esp+12]  ; ebp
	mov esp, [esp+16]  ; esp
	mov cr3, eax       ; set the page directory
	mov eax, 0x1       ; magic number to detect a task switch
	sti
	jmp ecx

become_user:
	cli
	mov ax, [esp+4]
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	mov ebx, esp
	push DWORD [ebx+4]
	push DWORD [ebx+8]
	pushf
	pop eax
	or eax, 0x200
	push eax
	push DWORD [ebx+12]
	push DWORD [ebx+16]
	iret
