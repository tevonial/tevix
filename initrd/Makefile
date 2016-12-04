all: rdgen

rdgen: rdgen.c
	gcc -g -Wall -std=gnu99 -o $@ $<

clean:
	rm rdgen initrd

install: rdgen
	./rdgen src initrd
	cp initrd $(DESTDIR)$(BOOTDIR)
