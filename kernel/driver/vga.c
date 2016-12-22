#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <driver/vga.h>
#include <core/port.h>

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xC00B8000;

static size_t vga_row;
static size_t vga_column;
static uint8_t vga_color;
static uint16_t* vga_buffer;

void vga_init(void) {
	vga_row = 0;
	vga_column = 0;
	vga_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	vga_buffer = VGA_MEMORY;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			vga_buffer[index] = vga_entry(' ', vga_color);
		}
	}
}

void vga_setcolor(uint8_t color) {
	vga_color = color;
}

void vga_update_cursor(void) {
    unsigned temp;

    temp = vga_row * VGA_WIDTH + vga_column;

    outportb(0x3D4, 14);
    outportb(0x3D5, temp >> 8);
    outportb(0x3D4, 15);
    outportb(0x3D5, temp);
}

void vga_scroll() {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            const size_t nextrow_index = index + VGA_WIDTH;

            if (y == VGA_HEIGHT) //Last line, make it blank
                vga_buffer[index] = vga_entry(' ', vga_color);
            else
                vga_buffer[index] = vga_buffer[nextrow_index];
        }
    }
    vga_row = VGA_HEIGHT-1;
    vga_column = 0;
}

void vga_putch(char c) {
	if (c == '\n') {
	    vga_row++;
	    vga_column = 0;
	} else {
		//unsigned char uc = c;
		vga_buffer[vga_row * VGA_WIDTH + vga_column] = vga_entry(c, vga_color);

		if (++vga_column == VGA_WIDTH) {
			vga_column = 0;
			vga_row++;
		}
	}

	if (vga_row == VGA_HEIGHT)
		vga_scroll();

	vga_update_cursor();
}

void vga_puts(const char* data) {
	for (size_t i = 0; i < strlen(data); i++)
		vga_putch(data[i]);
}

void vga_put_hex(uint32_t n) {
    int32_t tmp;

    vga_puts("0x");

    char noZeroes = 1;

    int i;
    for (i = 28; i > 0; i -= 4) {
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && noZeroes != 0)
            continue;
    
        if (tmp >= 0xA) {
            noZeroes = 0;
            vga_putch(tmp-0xA+'a' );
        } else {
            noZeroes = 0;
            vga_putch( tmp+'0' );
        }
    }
  
    tmp = n & 0xF;

    if (tmp >= 0xA) {
        vga_putch(tmp-0xA+'a');
    } else {
        vga_putch(tmp+'0');
    }

}

void vga_put_dec(uint32_t n) {

    if (n == 0) {
        vga_putch('0');
        return;
    }

    int32_t acc = n;
    char c[32];
    int i = 0;
    while (acc > 0) {
        c[i] = '0' + acc%10;
        acc /= 10;
        i++;
    }
    c[i] = 0;

    char c2[32];
    c2[i--] = 0;
    int j = 0;
    while(i >= 0) {
        c2[i--] = c[j++];
    }
    vga_puts(c2);

}