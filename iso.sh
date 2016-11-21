#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/tevix.bin isodir/boot/tevix.bin
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "tevix" {
	multiboot /boot/tevix.bin
}
EOF
grub-mkrescue -o tevix.iso isodir
