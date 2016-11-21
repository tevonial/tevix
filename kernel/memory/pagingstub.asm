section .text

global load_page_dir
global disable_pse
global get_faulting_address

load_page_dir:
    push ebp ; Preserve ebp on the stack
    mov ebp, esp ; Store esp's original state in ebp

    mov eax, [ebp+8] ; Move the first parameter (page directory location)
    mov cr3, eax ; Move page directory location to cr3

    mov esp, ebp ; Put esp back to its original state
    pop ebp ; Restore ebp's original state from the stack
    ret

disable_pse:
    mov eax, cr4
    xor eax, 0x00000010 ; PSE bit
    mov cr4, eax        ; disable 4MiB pages


; Get value of cr2
get_faulting_address:
    mov eax, cr2
    ret