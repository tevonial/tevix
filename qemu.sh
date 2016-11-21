#!/bin/sh
set -e

case $1 in
	kernel)	qemu-system-x86_64 -kernel sysroot/boot/tevix.bin;;
	"")	. ./iso.sh
		qemu-system-x86_64 -cdrom tevix.iso;;
esac
