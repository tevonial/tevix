#include <stdio.h>

#if defined(__is_libk)
#include <driver/vga.h>
#endif

int putchar(int ic) {
#if defined(__is_libk)
	char c = (char) ic;
	vga_putch(c);
#else
	// TODO: Implement stdio and the write system call.
#endif
	return ic;
}
