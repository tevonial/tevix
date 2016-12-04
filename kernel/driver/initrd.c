#include <string.h>
#include <stdio.h>

#include <driver/initrd.h>

initrd_header_t *initrd_header;     // The header.
initrd_file_header_t *file_headers; // The list of file headers.
fs_node_t *initrd_root;             // Our root directory node.
fs_node_t *initrd_dev;              // We also add a directory node for /dev, so we can mount devfs later on.
fs_node_t *root_nodes;              // List of file nodes.
int nroot_nodes;                    // Number of file nodes.

struct dirent dirent;

static uint32_t initrd_read(fs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    uint32_t header_index = node->inode - 2;
    initrd_file_header_t *header = &file_headers[header_index];

    if (offset > header->length)
        return 0;

    if (offset + size > header->length)
        size = header->length - offset;

    memcpy(buffer, (void *)(header->offset + offset), size);
    return size;
}

static struct dirent *initrd_readdir(fs_node_t *node, uint32_t index) {
    // Return entry for initrd_dev
    if (node == initrd_root && index == 0) {
      strcpy(dirent.name, initrd_dev->name);
      dirent.ino = initrd_dev->inode;
      return &dirent;
    } 

    if (index > nroot_nodes)
        return 0;

    strcpy(dirent.name, root_nodes[index-1].name);
    dirent.name[strlen(root_nodes[index-1].name)] = 0;
    dirent.ino = root_nodes[index-1].inode;
    return &dirent;
}

static fs_node_t *initrd_finddir(fs_node_t *node, uint32_t inode) {
    if (node == initrd_root && inode == 0)
        return initrd_root;

    if (node == initrd_root && inode == 1)
        return initrd_dev;

    if (node == initrd_root && inode - 2 < nroot_nodes)
        return &root_nodes[inode - 2];

    return 0;
}

fs_node_t *initrd_init(uint32_t location) {
    // Initialise the main and file header pointers and populate the root directory.
    initrd_header = (initrd_header_t *)location;
    file_headers = (initrd_file_header_t *) ((uint32_t)location+sizeof(initrd_header_t));

    if (initrd_header->magic != INITRD_MAGIC) {
        printf("Invalid initial ramdisk");
        abort();
    }

    // Initialise the root directory.
    initrd_root = (fs_node_t*)kmalloc(sizeof(fs_node_t));
    strcpy(initrd_root->name, "initrd");
    initrd_root->mask = initrd_root->uid = initrd_root->gid = initrd_root->length = 0;
    initrd_root->inode = 0;
    initrd_root->flags = FS_DIRECTORY;
    initrd_root->read = 0;
    initrd_root->write = 0;
    initrd_root->open = 0;
    initrd_root->close = 0;
    initrd_root->readdir = &initrd_readdir;
    initrd_root->finddir = &initrd_finddir;
    initrd_root->ptr = 0;
    initrd_root->impl = 0;

    // Initialise the /dev directory (required!)
    initrd_dev = (fs_node_t*)kmalloc(sizeof(fs_node_t));
    strcpy(initrd_dev->name, "dev");
    initrd_dev->mask = initrd_dev->uid = initrd_dev->gid = initrd_dev->length = 0;
    initrd_dev->inode = 1;
    initrd_dev->flags = FS_DIRECTORY;
    initrd_dev->read = 0;
    initrd_dev->write = 0;
    initrd_dev->open = 0;
    initrd_dev->close = 0;
    initrd_dev->readdir = &initrd_readdir;
    initrd_dev->finddir = &initrd_finddir;
    initrd_dev->ptr = 0;
    initrd_dev->impl = 0;

    root_nodes = (fs_node_t*)kmalloc(sizeof(fs_node_t) * initrd_header->size);
    nroot_nodes = initrd_header->size;

    // Create node for each file in initrd
    for (uint32_t i = 0; i < initrd_header->size; i++) {
        // Edit the file's header - currently it holds the file offset
        // relative to the start of the ramdisk. We want it relative to the start
        // of memory.
        file_headers[i].offset += location;
        // Create a new file node.
        strcpy(root_nodes[i].name, &file_headers[i].name);
        root_nodes[i].mask = root_nodes[i].uid = root_nodes[i].gid = 0;
        root_nodes[i].length = file_headers[i].length;
        root_nodes[i].inode = i + 2;
        root_nodes[i].flags = FS_FILE;
        root_nodes[i].read = &initrd_read;
        root_nodes[i].write = 0;
        root_nodes[i].readdir = 0;
        root_nodes[i].finddir = 0;
        root_nodes[i].open = 0;
        root_nodes[i].close = 0;
        root_nodes[i].impl = 0;
    }
    return initrd_root;
}
