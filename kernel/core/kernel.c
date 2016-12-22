#include <core/gdt.h>
#include <core/interrupt.h>
#include <memory/memory.h>
#include <driver/vga.h>
#include <driver/initrd.h>
#include <task/task.h>


void kernel_main(void) {
	gdt_init();
	idt_init();
	vga_init();
	mem_init();
	paging_init();

	mem_print_reserved();

	syscall_init();
	task_init();

	fs_root = initrd_init(meminfo.initrd_start);
	printf("fs_root:\n");
    int i = 0;
	struct dirent *entry;
	while ( (entry = fs_readdir(fs_root, i)) != 0) {
		fs_node_t *node = fs_finddir(fs_root, entry->name);

		if (node->flags == FS_DIRECTORY)
			printf("/%s\n", node->name);
		else {
			printf("/%s [%dB]", node->name, node->length);
			char buf[256];
			fs_read(node, 0, node->length, buf);
			buf[node->length] = '\0';
			printf("\n    %s\n", buf);
		}
		i++;
	}


	fork();

	printf("Loading /helloworld.bin\n");
	exec("helloworld.bin");

	for (;;);
}