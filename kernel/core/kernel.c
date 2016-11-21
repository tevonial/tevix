#include <core/gdt.h>
#include <core/interrupt.h>
#include <driver/vga.h>
#include <memory/memory.h>
#include <memory/paging.h>

#include <stdio.h>

void kernel_main(void) {
	gdt_init();
	idt_init();
	vga_init();

	mem_init();
	mem_print_reserved();

	paging_init();

	char *ptr = (char*) 0x0f040aa0;
	map_page(ptr, PT_RW);
	memcpy(ptr, "Hello Paging!", 13);

	printf("String at 0x%x : %s\n", ptr, ptr);

	for (;;);
}
