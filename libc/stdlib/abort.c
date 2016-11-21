#include <stdio.h>
#include <stdlib.h>
#include <driver/vga.h>

__attribute__((__noreturn__))
void abort(void) {
	vga_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
#if defined(__is_libk)
	// TODO: Add proper kernel panic.
	printf("\nkernel panic: abort()");
#else
	// TODO: Abnormally terminate the process as if by SIGABRT.
	printf("abort()");
#endif
	while (1) { }
	__builtin_unreachable();
}
