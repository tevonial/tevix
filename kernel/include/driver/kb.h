#ifndef __DRIVER_KB_H
#define __DRIVER_KB_H

#include <stdint.h>
#include <stdbool.h>

#include <core/port.h>

#define KB_ENC     0x60
#define KB_CTRL    0x64

#define KB_STATUS_OUT 1
#define KB_STATUS_IN  2



extern void kb_handler();
static void shift_key();

extern unsigned char kb_chetchar();
extern uint32_t kb_gets(char *buf);

extern void kb_enc_cmd(uint8_t cmd);
extern void kb_ctrl_cmd(uint8_t cmd);

static inline uint8_t kb_enc_read() {
	return inportb(KB_ENC);
}

static inline uint8_t kb_ctrl_read() {
	return inportb(KB_CTRL);
}

#endif