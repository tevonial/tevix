#include <driver/fs.h>

fs_node_t *fs_root = 0; // The root of the filesystem.

uint32_t fs_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node->read != 0)
        return node->read(node, offset, size, buffer);

    return 0;
}

uint32_t fs_write(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    if (node->write != 0)
        return node->write(node, offset, size, buffer);
    
    return 0;
}

void fs_open(fs_node_t *node, uint8_t read, uint8_t write) {
    if (node->open != 0)
        return node->open(node);
}

void fs_close(fs_node_t *node) {
    if (node->close != 0)
        return node->close(node);
}

// Return entry of a directory node at the specified index
struct dirent *fs_readdir(fs_node_t *node, uint32_t index) {
    if ( (node->flags&0x7) == FS_DIRECTORY && node->readdir != 0 )
        return node->readdir(node, index);
    
    return 0;
}

// Return child node with the specified name
fs_node_t *fs_finddir(fs_node_t *node, char *name) {
    if ( (node->flags&0x7) == FS_DIRECTORY && node->finddir != 0 )
        return node->finddir(node, name);
    
    return 0;
}
