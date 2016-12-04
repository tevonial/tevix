SYSTEM_HEADER_PROJECTS="libc kernel"
PROJECTS="libc kernel initrd"

export PATH="$HOME/opt/cross/bin:$PATH"

export MAKE=make
export HOST=i686-elf

export AR=${HOST}-ar
export AS=nasm
export CC=${HOST}-gcc

export BOOTDIR=/boot
export LIBDIR=/usr/lib
export INCLUDEDIR=/usr/include

export CFLAGS='-O2 -g'
export CPPFLAGS=''

# Configure the cross-compiler to use the desired system root.
export SYSROOT="$(pwd)/sysroot"
export CC="$CC --sysroot=$SYSROOT"

# Work around that the -elf gcc targets doesn't have a system include directory
# because it was configured with --without-headers rather than --with-sysroot.
if echo "$HOST" | grep -Eq -- '-elf($|-)'; then
  export CC="$CC -isystem=$INCLUDEDIR"
fi
