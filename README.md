# ecosSWtools

This contains a copy of [ecosSWtools-990319-src.tar.bz](https://gcc.gnu.org/pub/ecos/releases/ecos-1.2.1/ecosSWtools-990319-src.tar.bz),
which, as far as I know, contains the only known publicly available sources for the Zilog Z8000 (z8k-coff) and Mitsubishi D10V (d10v-elf)
GCC targets. With a little effort toolchains for both targets can be built on a contemporary Linux/x86-64 host.

## Building a z8k-coff cross toolchain

1. Due to the age of these sources you need an old compiler to build them. gcc-2.95.3 is known to work, gcc-3.x will _not_ work.
   You need kernel and libc support for 32-bit executables.
   I will assume this compiler is installed in `/opt/gcc-2.95.3`. (See below for how to build it.)
2. Check out this repository, pull the head of the `main` branch, create a separate build directory, and `cd` to the build directory.
3. Configure the build:

```
  PATH=/otp/gcc-2.95.3/bin:/usr/bin /path/to/ecosSWtools/src/configure --target=z8k-coff --build=i386-linux --host=i386-linux --prefix=/opt/cross-z8k --with-sysroot=/opt/cross-z8k --disable-shared --disable-nls
```

4. Remove unneeded or unbuildable components (sim and gdb do not build):

```
  rm -rf dejagnu diff expect gdb make mmalloc patch readline sim tcl texinfo tk utils
```

5. Perform the build:

```
  PATH=/opt/gcc-2.95.3/bin:/usr/bin make MAKEINFO=/bin/true LANGUAGES=c
```

6. Remove build-time tools we do not want installed:

```
  rm -rf bison byacc flex
```

7. Install the toolchain:

```
  PATH=/opt/gcc-2.95.3/bin:/usr/bin make MAKEINFO=/bin/true LANGUAGES=c install
```

8. z8k-coff binutils and newlib can be replaced with slightly newer versions from upstream sources, but
   current versions do not work with the ancient z8k-coff gcc.

## Building a d10v-elf cross toolchain

Use the same procedure as for z8k-coff, replacing `z8k-coff` with `d10v-elf`.

## Building a 32-bit gcc-2.95.3 compiler

The problem here is that gcc-2.95.3 predates x86_64 support in GCC, so we must build it for 32-bit x86.
It also needs a private copy of binutils that defaults to 32-bit x86.

1. Build 32-bit binutils from a separate build directory, any recent version should work (2.36.1 shown here):

```
  CFLAGS=-m32 /path/to/binutils-2.36.1/configure --build=i686-pc-linux-gnu --prefix=/opt/gcc-2.95.3/usr --disable-gold --disable-nls --disable-plugin
  make MAKEINFO=/bin/true
  make MAKEINFO=/bin/true install
```

2. Create a `gcc32` wrapper script:

```
  cat > /tmp/gcc32
  #!/bin/sh
  exec gcc -m32 -fgnu89-inline "$@"
  ^D
  chmod +x /tmp/gcc32
```

3. Build gcc-2.95.3 from a separate build directory, the bootstrap compiler can be any version up to gcc-11.1:

```
  CC=/tmp/gcc32 /path/to/gcc-2.95.3/configure --host=i686-pc-linux-gnu --prefix=/opt/gcc-2.95.3 --with-gnu-as --with-as=/opt/gcc-2.95.3/usr/bin/as --with-gnu-ld --with-ld=/opt/gcc-2.95.3/usr/bin/ld --disable-nls --disable-shared
  make MAKEINFO=/bin/true LANGUAGES=c bootstrap
  make MAKEINFO=/bin/true LANGUAGES=c install
```

## Copyright

For the copyright of ecosSWtools, see `COPYING` and `COPYING.LIB` in the `src` directory.

This README and changes to ecosSWtools are

    Copyright (C) 2021  Mikael Pettersson

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
