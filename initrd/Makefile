all: fs_gen

fs_gen: fs_gen.c
	gcc -g -Wall -std=gnu99 -o $@ $<

clean:
	rm fs_gen initrd

install: fs_gen
	./fs_gen src initrd
	cp initrd $(DESTDIR)$(BOOTDIR)
