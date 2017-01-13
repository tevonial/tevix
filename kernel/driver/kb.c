#include <stdio.h>

#include <core/interrupt.h>
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
static const unsigned char shift_map[] = {
    '\"', 0, 0, 0, 0,
    '<', '_', '>', '?', ')', '!', '@', '#', '$', '%', '^', '&', '*', '(',
    0, ':', 0, '+',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '{', '|', '}', 0, 0, '~'
};

enum special_key {
    L_SHIFT = 0x2A,
    R_SHIFT = 0x36,
    CAPS_LOCK = 0x3A,
    ALT = 0x38,
    CTRL = 0x1D
};

static volatile unsigned char buf[100];
static volatile uint8_t buf_i = 0;
static volatile uint8_t buf_len = 0;

// Keyboard modifier status
static volatile bool caps, shift, ctrl, alt;



void kb_init() {
    irq_install_handler(1, kb_handler);
}


void kb_handler() {
    uint8_t scancode = kb_enc_read();

    if (scancode & 0x80) { // Key released
        scancode ^= 0x80;

        if (scancode == L_SHIFT || scancode == R_SHIFT)
            shift = false;

    } else {
        // Non-character key pressed
        if (key_map[scancode] == 0) {
            if (scancode == L_SHIFT || scancode == R_SHIFT)
                shift = true;
            else if (scancode == CAPS_LOCK)
                caps = !caps;

            return; // Nothing more to do
        }

        // Character key was pressed
        uint8_t i = buf_i + buf_len++;
        if (i == 100)
            i -= 100;


        if (shift | caps)
            buf[i] = shift_key(key_map[scancode]);
        else
            buf[i] = key_map[scancode];
        
    }
}

static unsigned char shift_key(unsigned char c) {
    // Capitalize (if shift and caps not both set)
    if (c >= 'a' && c <= 'z' && (shift ^ caps))
        return c - 32;

    // Symbol (if shift set)
    else if (c >= '\'' && c <= '`' && shift)
        return shift_map[c - '\''];


    return c;
}

// Wait for buffer, return first char
unsigned char kb_getchar() {
    unsigned char c;

    while (!buf_len);

    c = buf[buf_i];

    buf_len--;
    if (++buf_i == 100)
        buf_i = 0;

    return c;
}

// Get string of chars terminated by newline, returns length
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
    while (kb_ctrl_read() & (1 << 2));

    outportb(KB_ENC, cmd);
}

void kb_ctrl_cmd(uint8_t cmd) {
    while (kb_ctrl_read() & (1 << 2));

    outportb(KB_CTRL, cmd);
}