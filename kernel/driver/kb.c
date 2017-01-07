#include <stdio.h>

#include <driver/kb.h>
#include <driver/vga.h>

// Maps scancodes to ascii values
static const unsigned char key_map[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',
    '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
    'm', ',', '.', '/',   0, '*', 0, ' ',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-', 0, 0, 0, '+',
};

// Maps ascii values to their shifted counterparts, starting at double colon
static const unsigned char shifted_key_map[] = {
    '\"', 0, 0, 0, 0,
    '<', '_', '>', '?', ')', '!', '@', '#', '$', '%', '^', '&', '*', '(',
    0, ':', 0, '+',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '{', '|', '}', 0, 0, '~'
};

enum special_key {
    L_SHIFT = 0x2A,
    R_SHIFT = 0x36,
    CAPS_LOCK = 0x3A
};


volatile static unsigned char lastkey = 0;

volatile static bool caps, shift, ctrl;



void kb_init() {
    irq_install_handler(1, kb_handler);
}


void kb_handler() {
    uint8_t scancode = kb_enc_read();

    if (scancode & 0x80) {
        scancode ^= 0x80;

        if (scancode == L_SHIFT || scancode == R_SHIFT)
                shift = false;
        else if (scancode == CAPS_LOCK)
            caps = true;

    } else {
        // Special key pressed
        if (key_map[scancode] == 0) {
            if (scancode == L_SHIFT || scancode == R_SHIFT)
                shift = true;

            return; // Nothing more to do
        }

        // Character key was pressed
        lastkey = key_map[scancode];

        if (shift ^ caps)
            shift_key();
        
    }
}

static void shift_key() {
    if (lastkey >= 'a' && lastkey <= 'z')
        lastkey -= 32;
    else if (lastkey >= '\'' && lastkey <= '`')
        lastkey = shifted_key_map[lastkey - '\''];

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