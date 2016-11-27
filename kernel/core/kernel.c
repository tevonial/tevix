#include <core/gdt.h>
#include <core/interrupt.h>
#include <driver/vga.h>
#include <memory/memory.h>

#include <stdio.h>

void kernel_main(void) {
	gdt_init();
	idt_init();
	vga_init();
	mem_init();
	paging_init();

	mem_print_reserved();

	char *a = kmalloc(0x10);
	char *b = kmalloc(0x10);
	char *c = kmalloc(0x10);

	kfree(a);
	kfree(b);
	kmalloc(0x20);
	char *ptr = kmalloc(0xf1);

	memcpy(ptr, "Hello heap!", 11);
	printf("String at 0x%x: %s", ptr, ptr);


	for (;;);
}
