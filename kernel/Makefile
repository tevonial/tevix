CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc

include core/make.config
include driver/make.config
include memory/make.config

KERNEL_OBJS=\
$(CORE_OBJS) \
$(DRIVER_OBJS) \
$(MEMORY_OBJS) \

OBJS=\
core/crti.o \
core/crtbegin.o \
$(KERNEL_OBJS) \
core/crtend.o \
core/crtn.o \

LINK_LIST=\
$(LDFLAGS) \
core/crti.o \
core/crtbegin.o \
$(KERNEL_OBJS) \
$(LIBS) \
core/crtend.o \
core/crtn.o \

.PHONY: all clean install install-headers install-kernel
#.SUFFIXES: .o .c .asm

all: tevix.bin

tevix.bin: $(OBJS) linker.ld
	$(CC) -T linker.ld -o $@ $(CFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot tevix.bin

core/crtbegin.o core/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

%.o : %.c
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

%.o : %.asm
	${AS} -f elf -o $@ $<

clean:
	rm -f tevix.bin
	rm -f $(OBJS) *.o */*.o */*/*.o
	rm -f $(OBJS:.o=.d) *.d */*.d */*/*.d

install: install-headers install-kernel

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-kernel: tevix.bin
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp tevix.bin $(DESTDIR)$(BOOTDIR)

-include $(OBJS:.o=.d)
