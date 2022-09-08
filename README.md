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

gcc-2.95.3 predates x86-64 support in gcc, so it must be built for 32-bit x86.
It also needs a private copy of binutils that defaults to 32-bit x86.

Please see [build-gcc295](https://github.com/mikpe/build-gcc295) for build instructions.

## Copyright

For the copyright of ecosSWtools, see `COPYING` and `COPYING.LIB` in the `src` directory.

This README and changes to ecosSWtools are

    Copyright (C) 2021-2022  Mikael Pettersson

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
