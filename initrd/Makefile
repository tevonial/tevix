CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc

USER_PROGRAMS=\
src/helloworld.bin\


all: rdgen $(USER_PROGRAMS)

%.bin : %.asm
	${AS} -f bin -o $@ $<
	cp $@ root

%.bin : %.c
	${CC} -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS) $(LIBS)
	cp $@ root

rdgen: rdgen.c
	gcc -g -Wall -std=gnu99 -o $@ $<

clean:
	rm rdgen initrd $(USER_PROGRAMS) root/*.bin

install: rdgen $(USER_PROGRAMS)
	./rdgen root initrd
	cp initrd $(DESTDIR)$(BOOTDIR)
