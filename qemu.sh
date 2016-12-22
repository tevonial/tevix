#!/bin/sh
set -e

. ./iso.sh

case $1 in
	gdb) qemu-system-x86_64 -s -S -cdrom tevix.iso;;
	"")	 qemu-system-x86_64 -cdrom tevix.iso;;
esac
