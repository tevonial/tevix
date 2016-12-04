#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#define RD_MAGIC	0x02468ACE

struct {
	uint32_t magic;
	uint32_t size;
} rd_header;

typedef struct {
	char name[32];
	uint32_t offset;
	uint32_t length;
} file_descriptor_t;

int main (int argc, char **argv) {

	DIR *dir;
	FILE *f, *out;
	struct dirent *ep;

	int nfiles = 0;
	file_descriptor_t *header;

	dir = opendir(argv[1]);
	if (dir == NULL)
		return 2;

	// Count files to be used
	while (ep = readdir(dir)) {
		if (ep->d_name[0] != '.')
			nfiles++;
	}

	closedir(dir);

	rd_header.magic = RD_MAGIC;
	rd_header.size = nfiles;
	header = malloc(nfiles * sizeof(file_descriptor_t));

	char fp[64];				// Full path of file
	strcpy(fp, argv[1]);
	strcat(fp, "/");
	char *fn = fp + strlen(fp);	// Pointer to the just filename
	out = fopen(argv[2], "w+");
	

	// Skip header section
	fseek(out, sizeof(rd_header) + nfiles * sizeof(file_descriptor_t), SEEK_SET);
	dir = opendir(argv[1]);

	uint32_t i = 0;
	while (ep = readdir(dir)) {			// Add files one by one after header section
		if (ep->d_name[0] != '.') {
			strcpy(fn, ep->d_name);			// Construct this particular filepath
			f = fopen(fp, "r");
			
			header[i].offset = ftell(out);	// Add location of file to its header
			strcpy(header[i].name, fn);		// Add filename
			fseek(f, 0, SEEK_END);
			header[i].length = ftell(f);	// Add filesize
			fseek(f, 0, SEEK_SET);

			// Copy raw content of the file
			unsigned char *buffer = (unsigned char *)malloc(header[i].length);
			fread(buffer, 1, header[i].length, f);
			fwrite(buffer, 1, header[i].length, out);
			fclose(f);
			free(buffer);

			printf("%s to %d-%d\n", fn, header[i].offset, header[i].offset + header[i].length - 1);

			i++;
		}
	}

	
	// Add header to beginning of image
	fseek(out, 0, SEEK_SET);
	fwrite(&rd_header, sizeof(rd_header), 1, out);
	fwrite(header, sizeof(file_descriptor_t), nfiles, out);

	fclose(out);
	printf("Ramdisk \"%s\" created with %d files\n", argv[2], nfiles);

	return 0;
}