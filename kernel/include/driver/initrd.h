#ifndef __DRIVER_INITRD_H
#define __DRIVER_INITRD_H

#include <stdint.h>
#include <driver/fs.h>

#define INITRD_MAGIC	0x02468ACE

typedef struct {
	uint32_t magic;		// Make sure image is valid
	uint32_t size;		// Entries in image
} initrd_header_t;

typedef struct {
	char name[32];		// Name of file
	uint32_t offset;	// Start of file
	uint32_t length;	// Length of file
} initrd_file_header_t;


static uint32_t initrd_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
static struct dirent *initrd_readdir(fs_node_t *node, uint32_t index);

fs_node_t *initrd_init(uint32_t location);

#endif