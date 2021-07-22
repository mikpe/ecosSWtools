#! /bin/sh
# Generate CGEN opcode files: arch-opc.[ch], arch-asm.c, arch-asm.c.
#
# Usage: /bin/sh cgen.sh "opcodes" srcdir cgendir cgenflags \
#	scheme schemeflags arch
#
# We store the generated files in the source directory until we decide to
# ship a scheme with gdb/binutils.  Maybe we never will.

# We want to behave like make, any error forces us to stop.
set -e

# `action' is currently always "opcodes".
# It exists to be consistent with the simulator.

action=$1
srcdir=$2
cgendir=$3
cgenflags=$4
scheme=$5
schemeflags=$6
arch=$7

rootdir=${srcdir}/..

ARCH=`echo ${arch} | tr 'abcdefghijklmnopqrstuvwxyz'  'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`

case $action in
opcodes)
	rm -f tmp-opc.h tmp-opc.c tmp-asm.c tmp-dis.c
	rm -f tmp-opc.h1 tmp-opc.c1 tmp-asm.c1 tmp-dis.c1
	rm -f tmp-asm.in tmp-asm.in1 tmp-dis.in tmp-dis.in1

	${scheme} ${schemeflags} ${cgendir}/cgen-opc.scm \
		-s ${cgendir} \
		${cgenflags} \
		-m all \
		-a ${arch} \
		-h tmp-opc.h1 \
		-t tmp-opc.c1 \
		-A tmp-asm.in1 \
		-D tmp-dis.in1

	sed -e "s/@arch@/${arch}/g" < tmp-opc.h1 > tmp-opc.h
	${rootdir}/move-if-change tmp-opc.h ${srcdir}/${arch}-opc.h
	cat ${srcdir}/cgen-opc.in tmp-opc.c1 | \
	  sed -e "s/@ARCH@/${ARCH}/g" -e "s/@arch@/${arch}/g" > tmp-opc.c
	${rootdir}/move-if-change tmp-opc.c ${srcdir}/${arch}-opc.c
	sed -e "/ -- assembler routines/ r tmp-asm.in1" ${srcdir}/cgen-asm.in \
	  | sed -e "s/@arch@/${arch}/g" > tmp-asm.c
	${rootdir}/move-if-change tmp-asm.c ${srcdir}/${arch}-asm.c
	sed -e "/ -- disassembler routines/ r tmp-dis.in1" ${srcdir}/cgen-dis.in \
	  | sed -e "s/@arch@/${arch}/g" > tmp-dis.c
	${rootdir}/move-if-change tmp-dis.c ${srcdir}/${arch}-dis.c

	rm -f tmp-opc.h1 tmp-opc.c1 tmp-asm.c1 tmp-dis.c1
	rm -f tmp-asm.in tmp-asm.in1 tmp-dis.in tmp-dis.in1
	;;

*)
	echo "cgen.sh: bad action: ${action}" >&2
	exit 1
	;;

esac

exit 0
