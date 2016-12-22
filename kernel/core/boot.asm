; Declare constants for the multiboot header.
MBALIGN  equ  1<<0              ; align loaded modules on page boundaries
MEMINFO  equ  1<<1              ; provide memory map
FLAGS    equ  MBALIGN | MEMINFO ; this is the Multiboot 'flag' field
MAGIC    equ  0x1BADB002        ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + FLAGS)   ; checksum of above, to prove we are multiboot

VIRTUAL_BASE equ 0xC0000000

; Declare a multiboot header that marks the program as a kernel.
section .multiboot
align 4
	dd MAGIC
	dd FLAGS
	dd CHECKSUM
 


; Reserve space for stack
section .bss
align 4

global stack_bottom
global stack_top

stack_bottom:
	resb 8192
stack_top:

section .data
align 0x1000

KERNEL_PAGE equ (VIRTUAL_BASE >> 22)
BOOT_PAGE_DIR:
	dd 0x00000083
	times (KERNEL_PAGE - 1) dd 0
	dd 0x00000083
	times (1024 - KERNEL_PAGE - 1) dd 0


section .text
align 4

global _loader

_loader:

	mov ecx, (BOOT_PAGE_DIR - VIRTUAL_BASE)
	mov cr3, ecx

	mov ecx, cr4
	or ecx, 0x00000010	; PSE bit
	mov cr4, ecx		; enable 4MiB pages

	mov ecx, cr0
	or ecx, 0x80000000	; PG bit
	mov cr0, ecx		; enable paging

	lea ecx, [_high_start]
	jmp ecx

_high_start:

	mov esp, stack_top

    ; Push multiboot header and magic
	push ebx
    push eax

	; Call the global constructors.
	extern _init
	call _init

	extern _load_mbi
	call _load_mbi

	; Transfer control to the main kernel.
	extern kernel_main
	call kernel_main

	cli
.hang:
	hlt
	jmp .hang