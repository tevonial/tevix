#include <stdio.h>

#include <driver/kb.h>
#include <driver/vga.h>


unsigned char kbdus[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter lastkey */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 lastkey ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home lastkey */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End lastkey*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert lastkey */
    0,	/* Delete lastkey */
    0,   0,   0,
    0,	/* F11 lastkey */
    0,	/* F12 lastkey */
    0,	/* All other lastkeys are undefined */
};

enum special_key {
    L_SHIFT = 0x2A,
    R_SHIFT = 0x36
};


volatile static unsigned char lastkey = 0;

volatile static bool shift, ctrl;



void kb_init() {
    irq_install_handler(1, kb_handler);
}

void kb_handler() {

    uint8_t scancode = kb_enc_read();

    if (scancode & 0x80) {
        scancode ^= 0x80;
        if (scancode == L_SHIFT || scancode == R_SHIFT)
                shift = false;

    } else {
        if (kbdus[scancode] == 0) {
            if (scancode == L_SHIFT || scancode == R_SHIFT)
                shift = true;
        }

        lastkey = kbdus[scancode];

        if (shift && lastkey >= 'a' && lastkey <= 'z')
            lastkey -= 32;
    }
}

unsigned char kb_getchar() {
    unsigned char key;

    while (lastkey == 0);

    key = lastkey;
    lastkey = 0;

    return key;
}

uint32_t kb_gets(char *buf) {
    unsigned char key;
    uint32_t i = 0;

    while ((key = kb_getchar()) != '\n') {
        buf[i++] = key;
        vga_putch(key);
    } 
    buf[i] = '\0';

    return i;
}

void kb_enc_cmd(uint8_t cmd) {
	while (kb_ctrl_read() & KB_STATUS_IN);
 
	outportb(KB_ENC, cmd);
}

void kb_ctrl_cmd(uint8_t cmd) {
	while (kb_ctrl_read() & KB_STATUS_IN);
 
	outportb(KB_CTRL, cmd);
}