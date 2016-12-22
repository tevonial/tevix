#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/$KERNEL isodir/boot/
cp sysroot/boot/initrd isodir/boot/

cat > isodir/boot/grub/grub.cfg << EOF
menuentry "tevix" {
	multiboot /boot/$KERNEL
	module /boot/initrd
}
EOF
grub-mkrescue -o tevix.iso isodir
