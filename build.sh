#!/bin/sh
set -e
. ./config.sh

mkdir -p "$SYSROOT"

#Install project headers to sysroot
for PROJECT in $SYSTEM_HEADER_PROJECTS; do
  (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install-headers)
done

#Install project binaries to sysroot
for PROJECT in $PROJECTS; do
  (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install)
done
