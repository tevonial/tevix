#ifndef __CORE_PORT_H
#define __CORE_PORT_H

unsigned char inportb (unsigned short _port);
void outportb (unsigned short _port, unsigned char _data);

#endif