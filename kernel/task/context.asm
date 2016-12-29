section .text

global get_eip
global become_user

global switch_context

get_eip:
	pop eax
	jmp eax

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


switch_context:
	cli
	pop ebx				; pop return address

	mov ecx, [esp]		; thread_t to save state
	mov [ecx], ebp		; ebp
	mov [ecx+4], esp	; esp
	mov [ecx+8], eax	; eax
	mov eax, cr3
	mov [ecx+12], eax	; cr3
	mov [ecx+16], ebx	; return address

	mov ecx, [esp+4]	; thread_t to restore
	mov ebp, [ecx]		; ebp
	mov esp, [ecx+4]	; esp
	mov eax, [ecx+12]	; cr3
	mov cr3, eax
	mov eax, [ecx+8]	; eax

	push DWORD [ecx+16]	; return address

	sti
	ret


switch_context_old:
	cli
	mov ecx, [esp+4]   ; eip
	mov eax, [esp+8]   ; physical address of current directory
	mov ebp, [esp+12]  ; ebp
	mov esp, [esp+16]  ; esp
	mov cr3, eax       ; set the page directory
	mov eax, 0x1       ; magic number to detect a task switch
	sti
	jmp ecx