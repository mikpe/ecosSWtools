## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = cygnus

INCDIR = $(srcdir)/../include
BFDDIR = $(srcdir)/../bfd
DEP = mkdep

lib_LTLIBRARIES = libopcodes.la

# This is where bfd.h lives.
BFD_H = ../bfd/bfd.h

# Header files.
HFILES = \
	arm-opc.h \
	h8500-opc.h \
	sh-opc.h \
	sysdep.h \
	w65-opc.h \
	z8k-opc.h

# C source files that correspond to .o's.
CFILES = \
	a29k-dis.c \
	alpha-dis.c \
	alpha-opc.c \
	arm-dis.c \
	cgen-asm.c \
	cgen-dis.c \
	cgen-opc.c \
	d10v-dis.c \
	d10v-opc.c \
	dis-buf.c \
	disassemble.c \
	h8300-dis.c \
	h8500-dis.c \
	hppa-dis.c \
	i386-dis.c \
	i960-dis.c \
	m32r-asm.c \
	m32r-dis.c \
	m32r-opc.c \
	m68k-dis.c \
	m68k-opc.c \
	m88k-dis.c \
	mips-dis.c \
	mips-opc.c \
	mips16-opc.c \
	m10200-dis.c \
	m10200-opc.c \
	m10300-dis.c \
	m10300-opc.c \
	ns32k-dis.c \
	ppc-dis.c \
	ppc-opc.c \
	sh-dis.c \
	sparc-dis.c \
	sparc-opc.c \
	tic30-dis.c \
	w65-dis.c \
	z8k-dis.c \
	z8kgen.c

ALL_MACHINES = \
	a29k-dis.lo \
	alpha-dis.lo \
	alpha-opc.lo \
	arc-dis.lo \
	arc-opc.lo \
	arm-dis.lo \
	cgen-asm.lo \
	cgen-dis.lo \
	cgen-opc.lo \
	d10v-dis.lo \
	d10v-opc.lo \
	h8300-dis.lo \
	h8500-dis.lo \
	hppa-dis.lo \
	i386-dis.lo \
	i960-dis.lo \
	m32r-asm.lo \
	m32r-dis.lo \
	m32r-opc.lo \
	m68k-dis.lo \
	m68k-opc.lo \
	m88k-dis.lo \
	m10200-dis.lo \
	m10200-opc.lo \
	m10300-dis.lo \
	m10300-opc.lo \
	mips-dis.lo \
	mips-opc.lo \
	mips16-opc.lo \
	ppc-dis.lo \
	ppc-opc.lo \
	ns32k-dis.lo \
	sh-dis.lo \
	sparc-dis.lo \
	sparc-opc.lo \
	tic30-dis.lo \
	v850-dis.lo \
	v850-opc.lo \
	w65-dis.lo \
	z8k-dis.lo

OFILES = @BFD_MACHINES@

INCLUDES = -D_GNU_SOURCE -I. -I$(srcdir) -I../bfd -I$(INCDIR) -I$(BFDDIR) @HDEFINES@

disassemble.lo: disassemble.c $(INCDIR)/dis-asm.h
	$(LIBTOOL) --mode=compile $(COMPILE) -c @archdefs@ $(srcdir)/disassemble.c

libopcodes_la_SOURCES =  dis-buf.c disassemble.c
libopcodes_la_DEPENDENCIES = $(OFILES)
libopcodes_la_LIBADD = $(OFILES)
libopcodes_la_LDFLAGS = -release $(VERSION)

# libtool will build .libs/libopcodes.a.  We create libopcodes.a in
# the build directory so that we don't have to convert all the
# programs that use libopcodes.a simultaneously.  This is a hack which
# should be removed if everything else starts using libtool.  FIXME.

noinst_LIBRARIES = libopcodes.a

stamp-lib: libopcodes.la
	cp .libs/libopcodes.a libopcodes.tmp
	$(SHELL) $(srcdir)/../move-if-change libopcodes.tmp libopcodes.a
	touch stamp-lib

libopcodes.a: stamp-lib ; @true

CLEANFILES = libopcodes.a stamp-lib dep.sed .dep .dep1
CLEANFILES = libopcodes.a stamp-lib dep.sed .dep .dep1 stamp-m32r

# CGEN support is sani tized out of FSF releases for now.
# Sani tization must be split between assignments and rules because
# automake splits them that way.

SCHEME = @SCHEME@
SCHEMEFLAGS = -s
CGENDIR = $(srcdir)/../cgen
CGENFLAGS = -v

CGENFILES = $(CGENDIR)/object.scm $(CGENDIR)/utils.scm \
	$(CGENDIR)/attr.scm $(CGENDIR)/enum.scm $(CGENDIR)/types.scm \
	$(CGENDIR)/utils-cgen.scm $(CGENDIR)/cpu.scm \
	$(CGENDIR)/mode.scm $(CGENDIR)/mach.scm \
	$(CGENDIR)/model.scm $(CGENDIR)/hardware.scm \
	$(CGENDIR)/ifield.scm $(CGENDIR)/iformat.scm \
	$(CGENDIR)/operand.scm $(CGENDIR)/insn.scm \
	$(CGENDIR)/opcodes.scm $(CGENDIR)/cdl-c.scm \
	$(CGENDIR)/cgen-opc.scm cgen-opc.in cgen-asm.in cgen-dis.in
# The end marker is written this way to pass through automake unscathed.

cgen:
	$(SHELL) $(srcdir)/cgen.sh opcodes $(srcdir) $(CGENDIR) $(CGENFLAGS) $(SCHEME) $(SCHEMEFLAGS) $(arch)
	touch stamp-${arch}

.PHONY: cgen

# For now, require developers to configure with --enable-maintainer-mode.
# ??? Do we need to use a different option?
m32r-opc.h m32r-opc.c m32r-asm.c m32r-dis.c: @MAINT@ stamp-m32r
	@true
stamp-m32r: $(CGENFILES) $(CGENDIR)/m32r.cpu $(CGENDIR)/m32r.opc
	$(MAKE) cgen arch=m32r prefix=m32r



# This dependency stuff is copied from BFD.

.dep: dep.sed $(CFILES) $(HFILES) config.h
	rm -f .dep1
	$(MAKE) DEP=$(DEP) .dep1
	sed -f dep.sed < .dep1 > .dep

.dep1: $(CFILES)
	rm -f .dep2 .dep2a
	echo '# DO NOT DELETE THIS LINE -- mkdep uses it.' > .dep2
	echo > .dep2a
	$(DEP) -f .dep2a $(INCLUDES) $(CFLAGS) $?
	sed -e '/DO NOT DELETE/d' -e '/^$$/d' < .dep2a >> .dep2
	rm -f .dep2a
	$(srcdir)/../move-if-change .dep2 .dep1

dep.sed: dep-in.sed config.status
	sed <$(srcdir)/dep-in.sed >dep.sed	\
		-e 's!@BFD_H@!$(BFD_H)!'	\
		-e 's!@INCDIR@!$(INCDIR)!'	\
		-e 's!@BFDDIR@!$(BFDDIR)!'	\
		-e 's!@SRCDIR@!$(srcdir)!'

dep: .dep
	sed -e '/^..DO NOT DELETE THIS LINE/,$$d' < Makefile > tmp-Makefile
	cat .dep >> tmp-Makefile
	$(srcdir)/../move-if-change tmp-Makefile Makefile

dep-in: .dep
	sed -e '/^..DO NOT DELETE THIS LINE/,$$d' < $(srcdir)/Makefile.in > tmp-Makefile.in
	cat .dep >> tmp-Makefile.in
	$(srcdir)/../move-if-change tmp-Makefile.in $(srcdir)/Makefile.in

dep-am: .dep
	sed -e '/^..DO NOT DELETE THIS LINE/,$$d' < $(srcdir)/Makefile.am > tmp-Makefile.am
	cat .dep >> tmp-Makefile.am
	$(srcdir)/../move-if-change tmp-Makefile.am $(srcdir)/Makefile.am

.PHONY: dep dep-in dep-am

# What appears below is generated by a hacked mkdep using gcc -MM.

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.
a29k-dis.lo: a29k-dis.c $(INCDIR)/dis-asm.h $(BFD_H) \
  $(INCDIR)/ansidecl.h $(INCDIR)/opcode/a29k.h
alpha-dis.lo: alpha-dis.c $(INCDIR)/ansidecl.h sysdep.h \
  config.h $(INCDIR)/dis-asm.h $(BFD_H) $(INCDIR)/opcode/alpha.h
alpha-opc.lo: alpha-opc.c $(INCDIR)/ansidecl.h $(INCDIR)/opcode/alpha.h \
  $(BFD_H)
arm-dis.lo: arm-dis.c $(INCDIR)/dis-asm.h $(BFD_H) \
  $(INCDIR)/ansidecl.h arm-opc.h $(INCDIR)/coff/internal.h \
  $(BFDDIR)/libcoff.h $(INCDIR)/bfdlink.h
cgen-asm.lo: cgen-asm.c sysdep.h config.h $(INCDIR)/libiberty.h \
  $(BFD_H) $(INCDIR)/opcode/cgen.h
cgen-dis.lo: cgen-dis.c sysdep.h config.h $(INCDIR)/libiberty.h \
  $(BFD_H) $(INCDIR)/opcode/cgen.h
cgen-opc.lo: cgen-opc.c sysdep.h config.h $(INCDIR)/libiberty.h \
  $(BFD_H) $(INCDIR)/opcode/cgen.h
d10v-dis.lo: d10v-dis.c $(INCDIR)/ansidecl.h $(INCDIR)/opcode/d10v.h \
  $(INCDIR)/dis-asm.h $(BFD_H)
d10v-opc.lo: d10v-opc.c $(INCDIR)/ansidecl.h $(INCDIR)/opcode/d10v.h
dis-buf.lo: dis-buf.c sysdep.h config.h $(INCDIR)/dis-asm.h \
  $(BFD_H)
disassemble.lo: disassemble.c $(INCDIR)/ansidecl.h \
  $(INCDIR)/dis-asm.h $(BFD_H)
h8300-dis.lo: h8300-dis.c $(INCDIR)/opcode/h8300.h \
  $(INCDIR)/dis-asm.h $(BFD_H) $(INCDIR)/ansidecl.h
h8500-dis.lo: h8500-dis.c h8500-opc.h $(INCDIR)/dis-asm.h \
  $(BFD_H) $(INCDIR)/ansidecl.h
hppa-dis.lo: hppa-dis.c sysdep.h config.h $(INCDIR)/dis-asm.h \
  $(BFD_H) $(BFDDIR)/libhppa.h $(INCDIR)/opcode/hppa.h
i386-dis.lo: i386-dis.c $(INCDIR)/dis-asm.h $(BFD_H) \
  $(INCDIR)/ansidecl.h sysdep.h config.h
i960-dis.lo: i960-dis.c $(INCDIR)/dis-asm.h $(BFD_H) \
  $(INCDIR)/ansidecl.h
m32r-asm.lo: m32r-asm.c sysdep.h config.h $(BFD_H) \
  m32r-opc.h $(INCDIR)/opcode/cgen.h
m32r-dis.lo: m32r-dis.c sysdep.h config.h $(INCDIR)/dis-asm.h \
  $(BFD_H) m32r-opc.h $(INCDIR)/opcode/cgen.h
m32r-opc.lo: m32r-opc.c sysdep.h config.h $(INCDIR)/libiberty.h \
  $(BFD_H) m32r-opc.h $(INCDIR)/opcode/cgen.h
m68k-dis.lo: m68k-dis.c $(INCDIR)/dis-asm.h $(BFD_H) \
  $(INCDIR)/ansidecl.h $(INCDIR)/floatformat.h $(INCDIR)/opcode/m68k.h
m68k-opc.lo: m68k-opc.c $(INCDIR)/ansidecl.h $(INCDIR)/opcode/m68k.h
m88k-dis.lo: m88k-dis.c $(INCDIR)/dis-asm.h $(BFD_H) \
  $(INCDIR)/ansidecl.h $(INCDIR)/opcode/m88k.h
mips-dis.lo: mips-dis.c sysdep.h config.h $(INCDIR)/dis-asm.h \
  $(BFD_H) $(INCDIR)/opcode/mips.h $(BFDDIR)/elf-bfd.h \
  $(INCDIR)/elf/common.h $(INCDIR)/elf/internal.h $(INCDIR)/elf/external.h \
  $(INCDIR)/bfdlink.h $(INCDIR)/elf/mips.h
mips-opc.lo: mips-opc.c $(INCDIR)/ansidecl.h $(INCDIR)/opcode/mips.h
mips16-opc.lo: mips16-opc.c $(INCDIR)/ansidecl.h $(INCDIR)/opcode/mips.h
m10200-dis.lo: m10200-dis.c $(INCDIR)/ansidecl.h $(INCDIR)/opcode/mn10200.h \
  $(INCDIR)/dis-asm.h $(BFD_H)
m10200-opc.lo: m10200-opc.c $(INCDIR)/ansidecl.h $(INCDIR)/opcode/mn10200.h
m10300-dis.lo: m10300-dis.c $(INCDIR)/ansidecl.h $(INCDIR)/opcode/mn10300.h \
  $(INCDIR)/dis-asm.h $(BFD_H)
m10300-opc.lo: m10300-opc.c $(INCDIR)/ansidecl.h $(INCDIR)/opcode/mn10300.h
ns32k-dis.lo: ns32k-dis.c $(BFD_H) $(INCDIR)/ansidecl.h \
  sysdep.h config.h $(INCDIR)/dis-asm.h $(INCDIR)/opcode/ns32k.h
ppc-dis.lo: ppc-dis.c $(INCDIR)/ansidecl.h sysdep.h \
  config.h $(INCDIR)/dis-asm.h $(BFD_H) $(INCDIR)/opcode/ppc.h
ppc-opc.lo: ppc-opc.c $(INCDIR)/ansidecl.h $(INCDIR)/opcode/ppc.h
sh-dis.lo: sh-dis.c sh-opc.h $(INCDIR)/dis-asm.h $(BFD_H) \
  $(INCDIR)/ansidecl.h
sparc-dis.lo: sparc-dis.c $(INCDIR)/ansidecl.h sysdep.h \
  config.h $(INCDIR)/opcode/sparc.h $(INCDIR)/dis-asm.h \
  $(BFD_H) $(INCDIR)/libiberty.h
sparc-opc.lo: sparc-opc.c $(INCDIR)/ansidecl.h $(INCDIR)/opcode/sparc.h
tic30-dis.lo: tic30-dis.c $(INCDIR)/dis-asm.h $(BFD_H) \
  $(INCDIR)/ansidecl.h $(INCDIR)/opcode/tic30.h
w65-dis.lo: w65-dis.c w65-opc.h $(INCDIR)/dis-asm.h \
  $(BFD_H) $(INCDIR)/ansidecl.h
z8k-dis.lo: z8k-dis.c sysdep.h config.h $(INCDIR)/dis-asm.h \
  $(BFD_H) z8k-opc.h
z8kgen.lo: z8kgen.c sysdep.h config.h
# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
