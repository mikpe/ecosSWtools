# This file contains common code used by all simulators.
#
# SIM_AC_COMMON invokes AC macros used by all simulators and by the common
# directory.  It is intended to be invoked before any target specific stuff.
# SIM_AC_OUTPUT is a cover function to AC_OUTPUT to generate the Makefile.
# It is intended to be invoked last.
#
# The simulator's configure.in should look like:
#
# dnl Process this file with autoconf to produce a configure script.
# sinclude(../common/aclocal.m4)
# AC_PREREQ(2.5)dnl
# AC_INIT(Makefile.in)
#
# SIM_AC_COMMON
#
# ... target specific stuff ...
#
# SIM_AC_OUTPUT

AC_DEFUN(SIM_AC_COMMON,
[
# autoconf.info says this should be called right after AC_INIT.
AC_CONFIG_HEADER(ifelse([$1],,config.h,[$1]):config.in)

AC_CONFIG_AUX_DIR(`cd $srcdir;pwd`/../..)
AC_CANONICAL_SYSTEM
AC_ARG_PROGRAM
AC_PROG_CC
AC_PROG_INSTALL

# Put a plausible default for CC_FOR_BUILD in Makefile.
if test "x$cross_compiling" = "xno"; then
  CC_FOR_BUILD='$(CC)'
else
  CC_FOR_BUILD=gcc
fi
AC_SUBST(CC_FOR_BUILD)

AC_SUBST(CFLAGS)
AC_SUBST(HDEFINES)
AR=${AR-ar}
AC_SUBST(AR)
AC_PROG_RANLIB

# Check for common headers.
# FIXME: Seems to me this can cause problems for i386-windows hosts.
# At one point there were hardcoded AC_DEFINE's if ${host} = i386-*-windows*.
AC_CHECK_HEADERS(stdlib.h string.h strings.h unistd.h time.h)
AC_CHECK_HEADERS(sys/time.h sys/resource.h)
AC_CHECK_HEADERS(fcntl.h fpu_control.h)
AC_CHECK_FUNCS(getrusage time sigaction __setfpucw)

# Check for socket libraries
AC_CHECK_LIB(socket, bind)
AC_CHECK_LIB(nsl, gethostbyname)

. ${srcdir}/../../bfd/configure.host

dnl Standard (and optional) simulator options.
dnl Eventually all simulators will support these.
dnl Do not add any here that cannot be supported by all simulators.
dnl Do not add similar but different options to a particular simulator,
dnl all shall eventually behave the same way.


dnl We don't use automake, but we still want to support
dnl --enable-maintainer-mode.
USE_MAINTAINER_MODE=no
AC_ARG_ENABLE(maintainer-mode,
[  --enable-maintainer-mode		Enable developer functionality.],
[case "${enableval}" in
  yes)	MAINT="" USE_MAINTAINER_MODE=yes ;;
  no)	MAINT="#" ;;
  *)	AC_MSG_ERROR("--enable-maintainer-mode does not take a value"); MAINT="#" ;;
esac
if test x"$silent" != x"yes" && test x"$MAINT" = x""; then
  echo "Setting maintainer mode" 6>&1
fi],[MAINT="#"])dnl
AC_SUBST(MAINT)


dnl This is a generic option to enable special byte swapping
dnl insns on *any* cpu.
AC_ARG_ENABLE(sim-bswap,
[  --enable-sim-bswap			Use Host specific BSWAP instruction.],
[case "${enableval}" in
  yes)	sim_bswap="-DWITH_BSWAP=1 -DUSE_BSWAP=1";;
  no)	sim_bswap="-DWITH_BSWAP=0";;
  *)	AC_MSG_ERROR("--enable-sim-bswap does not take a value"); sim_bswap="";;
esac
if test x"$silent" != x"yes" && test x"$sim_bswap" != x""; then
  echo "Setting bswap flags = $sim_bswap" 6>&1
fi],[sim_bswap=""])dnl
AC_SUBST(sim_bswap)


AC_ARG_ENABLE(sim-cflags,
[  --enable-sim-cflags=opts		Extra CFLAGS for use in building simulator],
[case "${enableval}" in
  yes)	 sim_cflags="-O2";;
  trace) AC_MSG_ERROR("Please use --enable-sim-debug instead."); sim_cflags="";;
  no)	 sim_cflags="";;
  *)	 sim_cflags=`echo "${enableval}" | sed -e "s/,/ /g"`;;
esac
if test x"$silent" != x"yes" && test x"$sim_cflags" != x""; then
  echo "Setting sim cflags = $sim_cflags" 6>&1
fi],[sim_cflags=""])dnl
AC_SUBST(sim_cflags)


dnl --enable-sim-debug is for developers of the simulator
dnl the allowable values are work-in-progress
AC_ARG_ENABLE(sim-debug,
[  --enable-sim-debug=opts		Enable debugging flags],
[case "${enableval}" in
  yes) sim_debug="-DDEBUG=7 -DWITH_DEBUG=7";;
  no)  sim_debug="-DDEBUG=0 -DWITH_DEBUG=0";;
  *)   sim_debug="-DDEBUG='(${enableval})' -DWITH_DEBUG='(${enableval})'";;
esac
if test x"$silent" != x"yes" && test x"$sim_debug" != x""; then
  echo "Setting sim debug = $sim_debug" 6>&1
fi],[sim_debug=""])dnl
AC_SUBST(sim_debug)


dnl --enable-sim-stdio is for users of the simulator
dnl It determines if IO from the program is routed through STDIO (buffered)
AC_ARG_ENABLE(sim-stdio,
[  --enable-sim-stdio			Specify whether to use stdio for console input/output.],
[case "${enableval}" in
  yes)	sim_stdio="-DWITH_STDIO=DO_USE_STDIO";;
  no)	sim_stdio="-DWITH_STDIO=DONT_USE_STDIO";;
  *)	AC_MSG_ERROR("Unknown value $enableval passed to --enable-sim-stdio"); sim_stdio="";;
esac
if test x"$silent" != x"yes" && test x"$sim_stdio" != x""; then
  echo "Setting stdio flags = $sim_stdio" 6>&1
fi],[sim_stdio=""])dnl
AC_SUBST(sim_stdio)


dnl --enable-sim-trace is for users of the simulator
dnl The argument is either a bitmask of things to enable [exactly what is
dnl up to the simulator], or is a comma separated list of names of tracing
dnl elements to enable.  The latter is only supported on simulators that
dnl use WITH_TRACE.
AC_ARG_ENABLE(sim-trace,
[  --enable-sim-trace=opts		Enable tracing flags],
[case "${enableval}" in
  yes)	sim_trace="-DTRACE=1 -DWITH_TRACE=-1";;
  no)	sim_trace="-DTRACE=0 -DWITH_TRACE=0";;
  [[-0-9]]*)
	sim_trace="-DTRACE='(${enableval})' -DWITH_TRACE='(${enableval})'";;
  [[a-z]]*)
	sim_trace=""
	for x in `echo "$enableval" | sed -e "s/,/ /g"`; do
	  if test x"$sim_trace" = x; then
	    sim_trace="-DWITH_TRACE='(TRACE_$x"
	  else
	    sim_trace="${sim_trace}|TRACE_$x"
	  fi
	done
	sim_trace="$sim_trace)'" ;;
esac
if test x"$silent" != x"yes" && test x"$sim_trace" != x""; then
  echo "Setting sim trace = $sim_trace" 6>&1
fi],[sim_trace=""])dnl
AC_SUBST(sim_trace)


dnl --enable-sim-profile
dnl The argument is either a bitmask of things to enable [exactly what is
dnl up to the simulator], or is a comma separated list of names of profiling
dnl elements to enable.  The latter is only supported on simulators that
dnl use WITH_PROFILE.
AC_ARG_ENABLE(sim-profile,
[  --enable-sim-profile=opts		Enable profiling flags],
[case "${enableval}" in
  yes)	sim_profile="-DPROFILE=1 -DWITH_PROFILE=-1";;
  no)	sim_profile="-DPROFILE=0 -DWITH_PROFILE=0";;
  [[-0-9]]*)
	sim_profile="-DPROFILE='(${enableval})' -DWITH_PROFILE='(${enableval})'";;
  [[a-z]]*)
	sim_profile=""
	for x in `echo "$enableval" | sed -e "s/,/ /g"`; do
	  if test x"$sim_profile" = x; then
	    sim_profile="-DWITH_PROFILE='(PROFILE_$x"
	  else
	    sim_profile="${sim_profile}|PROFILE_$x"
	  fi
	done
	sim_profile="$sim_profile)'" ;;
esac
if test x"$silent" != x"yes" && test x"$sim_profile" != x""; then
  echo "Setting sim profile = $sim_profile" 6>&1
fi],[sim_profile=""])dnl
AC_SUBST(sim_profile)


dnl Types used by common code
AC_TYPE_SIGNAL

dnl Detect exe extension
AM_EXEEXT

dnl These are available to append to as desired.
sim_link_files=
sim_link_links=

dnl Create tconfig.h either from simulator's tconfig.in or default one
dnl in common.
sim_link_links=tconfig.h
if test -f ${srcdir}/tconfig.in
then
  sim_link_files=tconfig.in
else
  sim_link_files=../common/tconfig.in
fi

# targ-vals.def points to the libc macro description file.
case "${target}" in
*-*-*) TARG_VALS_DEF=../common/nltvals.def ;;
esac
sim_link_files="${sim_link_files} ${TARG_VALS_DEF}"
sim_link_links="${sim_link_links} targ-vals.def"

]) dnl End of SIM_AC_COMMON


dnl Additional SIM options that can (optionally) be configured
dnl For optional simulator options, a macro SIM_AC_OPTION_* is defined.
dnl Simulators that wish to use the relevant option specify the macro
dnl in the simulator specific configure.in file between the SIM_AC_COMMON
dnl and SIM_AC_OUTPUT lines.


dnl Specify the running environment.
dnl If the simulator invokes this in its configure.in then without this option
dnl the default is the user environment and all are runtime selectable.
dnl If the simulator doesn't invoke this, only the user environment is
dnl supported.
dnl ??? Until there is demonstrable value in doing something more complicated,
dnl let's not.
AC_DEFUN(SIM_AC_OPTION_ENVIRONMENT,
[
AC_ARG_ENABLE(sim-environment,
[  --enable-sim-environment=environment	Specify mixed, user, virtual or operating environment.],
[case "${enableval}" in
  all | ALL)             sim_environment="-DWITH_ENVIRONMENT=ALL_ENVIRONMENT";;
  user | USER)           sim_environment="-DWITH_ENVIRONMENT=USER_ENVIRONMENT";;
  virtual | VIRTUAL)     sim_environment="-DWITH_ENVIRONMENT=VIRTUAL_ENVIRONMENT";;
  operating | OPERATING) sim_environment="-DWITH_ENVIRONMENT=OPERATING_ENVIRONMENT";;
  *)   AC_MSG_ERROR("Unknown value $enableval passed to --enable-sim-environment");
       sim_environment="";;
esac
if test x"$silent" != x"yes" && test x"$sim_environment" != x""; then
  echo "Setting sim environment = $sim_environment" 6>&1
fi],
[sim_environment="-DWITH_ENVIRONMENT=ALL_ENVIRONMENT"])dnl
])
AC_SUBST(sim_environment)


dnl Specify the alignment restrictions of the target architecture.
dnl Without this option all possible alignment restrictions are accommodated.
dnl arg[1] is hardwired target alignment
dnl arg[2] is default target alignment
AC_DEFUN(SIM_AC_OPTION_ALIGNMENT,
wire_alignment="[$1]"
default_alignment="[$2]"
[
AC_ARG_ENABLE(sim-alignment,
[  --enable-sim-alignment=align		Specify strict,  nonstrict or forced alignment of memory accesses.],
[case "${enableval}" in
  strict | STRICT)       sim_alignment="-DWITH_ALIGNMENT=STRICT_ALIGNMENT";;
  nonstrict | NONSTRICT) sim_alignment="-DWITH_ALIGNMENT=NONSTRICT_ALIGNMENT";;
  forced | FORCED)       sim_alignment="-DWITH_ALIGNMENT=FORCED_ALIGNMENT";;
  yes) if test x"$wire_alignment" != x; then
	 sim_alignment="-DWITH_ALIGNMENT=${wire_alignment}"
       else
         if test x"$default_alignment" != x; then
           sim_alignment="-DWITH_ALIGNMENT=${default_alignment}"
         else
	   echo "No hard-wired alignment for target $target" 1>&6
	   sim_alignment="-DWITH_ALIGNMENT=0"
         fi
       fi;;
  no)  if test x"$default_alignment" != x; then
	 sim_alignment="-DWITH_DEFAULT_ALIGNMENT=${default_alignment}"
       else
         if test x"$wire_alignment" != x; then
	   sim_alignment="-DWITH_DEFAULT_ALIGNMENT=${wire_alignment}"
         else
           echo "No default alignment for target $target" 1>&6
           sim_alignment="-DWITH_DEFAULT_ALIGNMENT=0"
         fi
       fi;;
  *)   AC_MSG_ERROR("Unknown value $enableval passed to --enable-sim-alignment"); sim_alignment="";;
esac
if test x"$silent" != x"yes" && test x"$sim_alignment" != x""; then
  echo "Setting alignment flags = $sim_alignment" 6>&1
fi],
[if test x"$default_alignment" != x; then
  sim_alignment="-DWITH_DEFAULT_ALIGNMENT=${default_alignment}"
else
  if test x"$wire_alignment" != x; then
    sim_alignment="-DWITH_ALIGNMENT=${wire_alignment}"
  else
    sim_alignment=
  fi
fi])dnl
])dnl
AC_SUBST(sim_alignment)


dnl Conditionally compile in assertion statements.
AC_DEFUN(SIM_AC_OPTION_ASSERT,
[
AC_ARG_ENABLE(sim-assert,
[  --enable-sim-assert			Specify whether to perform random assertions.],
[case "${enableval}" in
  yes)	sim_assert="-DWITH_ASSERT=1";;
  no)	sim_assert="-DWITH_ASSERT=0";;
  *)	AC_MSG_ERROR("--enable-sim-assert does not take a value"); sim_assert="";;
esac
if test x"$silent" != x"yes" && test x"$sim_assert" != x""; then
  echo "Setting assert flags = $sim_assert" 6>&1
fi],[sim_assert=""])dnl
])
AC_SUBST(sim_assert)



dnl --enable-sim-bitsize is for developers of the simulator
dnl It specifies the number of BITS in the target.
dnl arg[1] is the number of bits in a word
dnl arg[2] is the number assigned to the most significant bit
dnl arg[3] is the number of bits in an address
dnl arg[4] is the number of bits in an OpenFirmware cell.
dnl FIXME: this information should be obtained from bfd/archure
AC_DEFUN(SIM_AC_OPTION_BITSIZE,
wire_word_bitsize="[$1]"
wire_word_msb="[$2]"
wire_address_bitsize="[$3]"
wire_cell_bitsize="[$4]"
[AC_ARG_ENABLE(sim-bitsize,
[  --enable-sim-bitsize=N		Specify target bitsize (32 or 64).],
[sim_bitsize=
case "${enableval}" in
  64,63 | 64,63,* ) sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=64 -DWITH_TARGET_WORD_MSB=63";;
  32,31 | 32,31,* ) sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=31";;
  64,0 | 64,0,* ) sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=0";;
  32,0 | 64,0,* ) sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=0";;
  32) if test x"$wire_word_msb" != x -a x"$wire_word_msb" != x0; then
        sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=31"
      else
        sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=32 -DWITH_TARGET_WORD_MSB=0"
      fi ;;
  64) if test x"$wire_word_msb" != x -a x"$wire_word_msb" != x0; then
        sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=64 -DWITH_TARGET_WORD_MSB=63"
      else
        sim_bitsize="-DWITH_TARGET_WORD_BITSIZE=64 -DWITH_TARGET_WORD_MSB=0"
      fi ;;
  *)  AC_MSG_ERROR("--enable-sim-bitsize was given $enableval.  Expected 32 or 64") ;;
esac
# address bitsize
tmp=`echo "${enableval}" | sed -e "s/^[[0-9]]*,*[[0-9]]*,*//"`
case x"${tmp}" in
  x ) ;;
  x32 | x32,* ) sim_bitsize="${sim_bitsize} -DWITH_TARGET_ADDRESS_BITSIZE=32" ;;
  x64 | x64,* ) sim_bitsize="${sim_bitsize} -DWITH_TARGET_ADDRESS_BITSIZE=64" ;;
  * ) AC_MSG_ERROR("--enable-sim-bitsize was given address size $enableval.  Expected 32 or 64") ;;
esac
# cell bitsize
tmp=`echo "${enableval}" | sed -e "s/^[[0-9]]*,*[[0-9*]]*,*[[0-9]]*,*//"`
case x"${tmp}" in
  x ) ;;
  x32 | x32,* ) sim_bitsize="${sim_bitsize} -DWITH_TARGET_CELL_BITSIZE=32" ;;
  x64 | x64,* ) sim_bitsize="${sim_bitsize} -DWITH_TARGET_CELL_BITSIZE=64" ;;
  * ) AC_MSG_ERROR("--enable-sim-bitsize was given cell size $enableval.  Expected 32 or 64") ;;
esac
if test x"$silent" != x"yes" && test x"$sim_bitsize" != x""; then
  echo "Setting bitsize flags = $sim_bitsize" 6>&1
fi],
[sim_bitsize=""
if test x"$wire_word_bitsize" != x; then
  sim_bitsize="$sim_bitsize -DWITH_TARGET_WORD_BITSIZE=$wire_word_bitsize"
fi
if test x"$wire_word_msb" != x; then
  sim_bitsize="$sim_bitsize -DWITH_TARGET_WORD_MSB=$wire_word_msb"
fi
if test x"$wire_address_bitsize" != x; then
  sim_bitsize="$sim_bitsize -DWITH_TARGET_ADDRESS_BITSIZE=$wire_address_bitsize"
fi
if test x"$wire_cell_bitsize" != x; then
  sim_bitsize="$sim_bitsize -DWITH_TARGET_CELL_BITSIZE=$wire_cell_bitsize"
fi])dnl
])
AC_SUBST(sim_bitsize)



dnl --enable-sim-endian={yes,no,big,little} is for simulators
dnl that support both big and little endian targets.
dnl arg[1] is hardwired target endianness.
dnl arg[2] is default target endianness.
AC_DEFUN(SIM_AC_OPTION_ENDIAN,
[
wire_endian="[$1]"
default_endian="[$2]"
AC_ARG_ENABLE(sim-endian,
[  --enable-sim-endian=endian		Specify target byte endian orientation.],
[case "${enableval}" in
  b*|B*) sim_endian="-DWITH_TARGET_BYTE_ORDER=BIG_ENDIAN";;
  l*|L*) sim_endian="-DWITH_TARGET_BYTE_ORDER=LITTLE_ENDIAN";;
  yes)	 if test x"$wire_endian" != x; then
	   sim_endian="-DWITH_TARGET_BYTE_ORDER=${wire_endian}"
	 else
           if test x"$default_endian" != x; then
	     sim_endian="-DWITH_TARGET_BYTE_ORDER=${default_endian}"
	   else
	     echo "No hard-wired endian for target $target" 1>&6
	     sim_endian="-DWITH_TARGET_BYTE_ORDER=0"
	   fi
	 fi;;
  no)	 if test x"$default_endian" != x; then
	   sim_endian="-DWITH_DEFAULT_TARGET_BYTE_ORDER=${default_endian}"
	 else
	   if test x"$wire_endian" != x; then
	     sim_endian="-DWITH_DEFAULT_TARGET_BYTE_ORDER=${wire_endian}"
	   else
	     echo "No default endian for target $target" 1>&6
	     sim_endian="-DWITH_DEFAULT_TARGET_BYTE_ORDER=0"
	   fi
	 fi;;
  *)	 AC_MSG_ERROR("Unknown value $enableval for --enable-sim-endian"); sim_endian="";;
esac
if test x"$silent" != x"yes" && test x"$sim_endian" != x""; then
  echo "Setting endian flags = $sim_endian" 6>&1
fi],
[if test x"$default_endian" != x; then
  sim_endian="-DWITH_DEFAULT_TARGET_BYTE_ORDER=${default_endian}"
else
  if test x"$wire_endian" != x; then
    sim_endian="-DWITH_TARGET_BYTE_ORDER=${wire_endian}"
  else
    sim_endian=
  fi
fi])dnl
])
AC_SUBST(sim_endian)


dnl --enable-sim-hostendian is for users of the simulator when
dnl they find that AC_C_BIGENDIAN does not function correctly
dnl (for instance in a canadian cross)
AC_DEFUN(SIM_AC_OPTION_HOSTENDIAN,
[
AC_ARG_ENABLE(sim-hostendian,
[  --enable-sim-hostendian=end		Specify host byte endian orientation.],
[case "${enableval}" in
  no)	 sim_hostendian="-DWITH_HOST_BYTE_ORDER=0";;
  b*|B*) sim_hostendian="-DWITH_HOST_BYTE_ORDER=BIG_ENDIAN";;
  l*|L*) sim_hostendian="-DWITH_HOST_BYTE_ORDER=LITTLE_ENDIAN";;
  *)	 AC_MSG_ERROR("Unknown value $enableval for --enable-sim-hostendian"); sim_hostendian="";;
esac
if test x"$silent" != x"yes" && test x"$sim_hostendian" != x""; then
  echo "Setting hostendian flags = $sim_hostendian" 6>&1
fi],[
if test "x$cross_compiling" = "xno"; then
  AC_C_BIGENDIAN
  if test $ac_cv_c_bigendian = yes; then
    sim_hostendian="-DWITH_HOST_BYTE_ORDER=BIG_ENDIAN"
  else
    sim_hostendian="-DWITH_HOST_BYTE_ORDER=LITTLE_ENDIAN"
  fi
else
  sim_hostendian="-DWITH_HOST_BYTE_ORDER=0"
fi])dnl
])
AC_SUBST(sim_hostendian)


dnl --enable-sim-float is for developers of the simulator
dnl It specifies the presence of hardware floating point
dnl And optionally the bitsize of the floating point register.
dnl arg[1] specifies the presence (or absence) of floating point hardware
dnl arg[2] specifies the number of bits in a floating point register
AC_DEFUN(SIM_AC_OPTION_FLOAT,
[
default_sim_float="[$1]"
default_sim_float_bitsize="[$2]"
AC_ARG_ENABLE(sim-float,
[  --enable-sim-float			Specify that the target processor has floating point hardware.],
[case "${enableval}" in
  yes | hard)	sim_float="-DWITH_FLOATING_POINT=HARD_FLOATING_POINT";;
  no | soft)	sim_float="-DWITH_FLOATING_POINT=SOFT_FLOATING_POINT";;
  32)           sim_float="-DWITH_FLOATING_POINT=HARD_FLOATING_POINT -DWITH_TARGET_FLOATING_POINT_BITSIZE=32";;
  64)           sim_float="-DWITH_FLOATING_POINT=HARD_FLOATING_POINT -DWITH_TARGET_FLOATING_POINT_BITSIZE=64";;
  *)		AC_MSG_ERROR("Unknown value $enableval passed to --enable-sim-float"); sim_float="";;
esac
if test x"$silent" != x"yes" && test x"$sim_float" != x""; then
  echo "Setting float flags = $sim_float" 6>&1
fi],[
sim_float=
if test x"${default_sim_float}" != x""; then
  sim_float="-DWITH_FLOATING_POINT=${default_sim_float}"
fi
if test x"${default_sim_float_bitsize}" != x""; then
  sim_float="$sim_float -DWITH_TARGET_FLOATING_POINT_BITSIZE=${default_sim_float_bitsize}"
fi
])dnl
])
AC_SUBST(sim_float)


dnl The argument is the default cache size if none is specified.
AC_DEFUN(SIM_AC_OPTION_SCACHE,
[
default_sim_scache="ifelse([$1],,0,[$1])"
AC_ARG_ENABLE(sim-scache,
[  --enable-sim-scache=size		Specify simulator execution cache size.],
[case "${enableval}" in
  yes)	sim_scache="-DWITH_SCACHE=${default_sim_scache}";;
  no)	sim_scache="-DWITH_SCACHE=0" ;;
  [[0-9]]*) sim_cache=${enableval};;
  *)	AC_MSG_ERROR("Bad value $enableval passed to --enable-sim-scache");
	sim_scache="";;
esac
if test x"$silent" != x"yes" && test x"$sim_scache" != x""; then
  echo "Setting scache size = $sim_scache" 6>&1
fi],[sim_scache="-DWITH_SCACHE=${default_sim_scache}"])
])
AC_SUBST(sim_scache)


dnl The argument is the default model if none is specified.
AC_DEFUN(SIM_AC_OPTION_DEFAULT_MODEL,
[
default_sim_default_model="ifelse([$1],,0,[$1])"
AC_ARG_ENABLE(sim-default-model,
[  --enable-sim-default-model=model	Specify default model to simulate.],
[case "${enableval}" in
  yes|no) AC_MSG_ERROR("Missing argument to --enable-sim-default-model");;
  *)	sim_default_model="-DWITH_DEFAULT_MODEL='\"${enableval}\"'";;
esac
if test x"$silent" != x"yes" && test x"$sim_default_model" != x""; then
  echo "Setting default model = $sim_default_model" 6>&1
fi],[sim_default_model="-DWITH_DEFAULT_MODEL='\"${default_sim_default_model}\"'"])
])
AC_SUBST(sim_default_model)


dnl --enable-sim-hardware is for users of the simulator
dnl arg[1] Enable sim-hw by default? ("yes" or "no")
dnl arg[2] is a space separated list of devices that override the defaults
dnl arg[3] is a space separated list of extra target specific devices.
AC_DEFUN(SIM_AC_OPTION_HARDWARE,
[
if test x"[$1]" = x"yes"; then
  sim_hw_p=yes
else
  sim_hw_p=no
fi
if test "[$2]"; then
  hardware="core pal glue"
else
  hardware="core pal glue [$3]"
fi
sim_hw_cflags="-DWITH_HW=1"
sim_hw="$hardware"
sim_hw_objs="\$(SIM_COMMON_HW_OBJS) `echo $sim_hw | sed -e 's/\([[^ ]][[^ ]]*\)/dv-\1.o/g'`"
AC_ARG_ENABLE(sim-hardware,
[  --enable-sim-hardware=LIST		Specify the hardware to be included in the build.],
[
case "${enableval}" in
  yes)	sim_hw_p=yes;;
  no)	sim_hw_p=no;;
  ,*)   sim_hw_p=yes; hardware="${hardware} `echo ${enableval} | sed -e 's/,/ /'`";;
  *,)   sim_hw_p=yes; hardware="`echo ${enableval} | sed -e 's/,/ /'` ${hardware}";;
  *)	sim_hw_p=yes; hardware="`echo ${enableval} | sed -e 's/,/ /'`"'';;
esac
if test "$sim_hw_p" != yes; then
  sim_hw_objs=
  sim_hw_cflags="-DWITH_HW=0"
  sim_hw=
else
  sim_hw_cflags="-DWITH_HW=1"
  # remove duplicates
  sim_hw=""
  sim_hw_objs="\$(SIM_COMMON_HW_OBJS)"
  for i in x $hardware ; do
    case " $f " in
      x) ;;
      *" $i "*) ;;
      *) sim_hw="$sim_hw $i" ; sim_hw_objs="$sim_hw_objs dv-$i.o";;
    esac
  done
fi
if test x"$silent" != x"yes" && test "$sim_hw_p" = "yes"; then
  echo "Setting hardware to $sim_hw_cflags, $sim_hw, $sim_hw_objs"
fi],[
if test "$sim_hw_p" != yes; then
  sim_hw_objs=
  sim_hw_cflags="-DWITH_HW=0"
  sim_hw=
fi
if test x"$silent" != x"yes"; then
  echo "Setting hardware to $sim_hw_cflags, $sim_hw, $sim_hw_objs"
fi])dnl
])
AC_SUBST(sim_hw_cflags)
AC_SUBST(sim_hw_objs)
AC_SUBST(sim_hw)


dnl --enable-sim-inline is for users that wish to ramp up the simulator's
dnl performance by inlining functions.
dnl Guarantee that unconfigured simulators do not do any inlining
sim_inline="-DDEFAULT_INLINE=0"
AC_DEFUN(SIM_AC_OPTION_INLINE,
[
default_sim_inline="ifelse([$1],,,-DDEFAULT_INLINE=[$1])"
AC_ARG_ENABLE(sim-inline,
[  --enable-sim-inline=inlines		Specify which functions should be inlined.],
[sim_inline=""
case "$enableval" in
  no)		sim_inline="-DDEFAULT_INLINE=0";;
  0)		sim_inline="-DDEFAULT_INLINE=0";;
  yes | 2)	sim_inline="-DDEFAULT_INLINE=ALL_C_INLINE";;
  1)		sim_inline="-DDEFAULT_INLINE=INLINE_LOCALS";;
  *) for x in `echo "$enableval" | sed -e "s/,/ /g"`; do
       new_flag=""
       case "$x" in
	 *_INLINE=*)	new_flag="-D$x";;
	 *=*)		new_flag=`echo "$x" | sed -e "s/=/_INLINE=/" -e "s/^/-D/"`;;
	 *_INLINE)	new_flag="-D$x=ALL_C_INLINE";;
	 *)		new_flag="-D$x""_INLINE=ALL_C_INLINE";;
       esac
       if test x"$sim_inline" = x""; then
	 sim_inline="$new_flag"
       else
	 sim_inline="$sim_inline $new_flag"
       fi
     done;;
esac
if test x"$silent" != x"yes" && test x"$sim_inline" != x""; then
  echo "Setting inline flags = $sim_inline" 6>&1
fi],[
if test "x$cross_compiling" = "xno"; then
  if test x"$gcc" != "x" -a x"${default_sim_inline}" != "x" ; then
    sim_inline="${default_sim_inline}"
    if test x"$silent" != x"yes"; then
      echo "Setting inline flags = $sim_inline" 6>&1
    fi
  else
    sim_inline=""
  fi
else
  sim_inline="-DDEFAULT_INLINE=0"
fi])dnl
])
AC_SUBST(sim_inline)


AC_DEFUN(SIM_AC_OPTION_PACKAGES,
[
AC_ARG_ENABLE(sim-packages,
[  --enable-sim-packages=list		Specify the packages to be included in the build.],
[packages=disklabel
case "${enableval}" in
  yes)	;;
  no)	AC_MSG_ERROR("List of packages must be specified for --enable-sim-packages"); packages="";;
  ,*)   packages="${packages}${enableval}";;
  *,)   packages="${enableval}${packages}";;
  *)	packages="${enableval}"'';;
esac
sim_pk_src=`echo $packages | sed -e 's/,/.c pk_/g' -e 's/^/pk_/' -e 's/$/.c/'`
sim_pk_obj=`echo $sim_pk_src | sed -e 's/\.c/.o/g'`
if test x"$silent" != x"yes" && test x"$packages" != x""; then
  echo "Setting packages to $sim_pk_src, $sim_pk_obj"
fi],[packages=disklabel
sim_pk_src=`echo $packages | sed -e 's/,/.c pk_/g' -e 's/^/pk_/' -e 's/$/.c/'`
sim_pk_obj=`echo $sim_pk_src | sed -e 's/\.c/.o/g'`
if test x"$silent" != x"yes"; then
  echo "Setting packages to $sim_pk_src, $sim_pk_obj"
fi])dnl
])
AC_SUBST(sim_packages)


AC_DEFUN(SIM_AC_OPTION_REGPARM,
[
AC_ARG_ENABLE(sim-regparm,
[  --enable-sim-regparm=nr-parm		Pass parameters in registers instead of on the stack - x86/GCC specific.],
[case "${enableval}" in
  0*|1*|2*|3*|4*|5*|6*|7*|8*|9*) sim_regparm="-DWITH_REGPARM=${enableval}";;
  no)                            sim_regparm="" ;;
  yes)                           sim_regparm="-DWITH_REGPARM=3";;
  *)   AC_MSG_ERROR("Unknown value $enableval for --enable-sim-regparm"); sim_regparm="";;
esac
if test x"$silent" != x"yes" && test x"$sim_regparm" != x""; then
  echo "Setting regparm flags = $sim_regparm" 6>&1
fi],[sim_regparm=""])dnl
])
AC_SUBST(sim_regparm)


AC_DEFUN(SIM_AC_OPTION_RESERVED_BITS,
[
default_sim_reserved_bits="ifelse([$1],,1,[$1])"
AC_ARG_ENABLE(sim-reserved-bits,
[  --enable-sim-reserved-bits		Specify whether to check reserved bits in instruction.],
[case "${enableval}" in
  yes)	sim_reserved_bits="-DWITH_RESERVED_BITS=1";;
  no)	sim_reserved_bits="-DWITH_RESERVED_BITS=0";;
  *)	AC_MSG_ERROR("--enable-sim-reserved-bits does not take a value"); sim_reserved_bits="";;
esac
if test x"$silent" != x"yes" && test x"$sim_reserved_bits" != x""; then
  echo "Setting reserved flags = $sim_reserved_bits" 6>&1
fi],[sim_reserved_bits="-DWITH_RESERVED_BITS=${default_sim_reserved_bits}"])dnl
])
AC_SUBST(sim_reserved_bits)


AC_DEFUN(SIM_AC_OPTION_SMP,
[
default_sim_smp="ifelse([$1],,5,[$1])"
AC_ARG_ENABLE(sim-smp,
[  --enable-sim-smp=n			Specify number of processors to configure for (default ${default_sim_smp}).],
[case "${enableval}" in
  yes)	sim_smp="-DWITH_SMP=5" ; sim_igen_smp="-N 5";;
  no)	sim_smp="-DWITH_SMP=0" ; sim_igen_smp="-N 0";;
  *)	sim_smp="-DWITH_SMP=$enableval" ; sim_igen_smp="-N $enableval";;
esac
if test x"$silent" != x"yes" && test x"$sim_smp" != x""; then
  echo "Setting smp flags = $sim_smp" 6>&1
fi],[sim_smp="-DWITH_SMP=${default_sim_smp}" ; sim_igen_smp="-N ${default_sim_smp}"
if test x"$silent" != x"yes"; then
  echo "Setting smp flags = $sim_smp" 6>&1
fi])dnl
])
AC_SUBST(sim_smp)


AC_DEFUN(SIM_AC_OPTION_STDCALL,
[
AC_ARG_ENABLE(sim-stdcall,
[  --enable-sim-stdcall=type		Use an alternative function call/return mechanism - x86/GCC specific.],
[case "${enableval}" in
  no)		sim_stdcall="" ;;
  std*)		sim_stdcall="-DWITH_STDCALL=1";;
  yes)		sim_stdcall="-DWITH_STDCALL=1";;
  *)   AC_MSG_ERROR("Unknown value $enableval for --enable-sim-stdcall"); sim_stdcall="";;
esac
if test x"$silent" != x"yes" && test x"$sim_stdcall" != x""; then
  echo "Setting function call flags = $sim_stdcall" 6>&1
fi],[sim_stdcall=""])dnl
])
AC_SUBST(sim_stdcall)


AC_DEFUN(SIM_AC_OPTION_XOR_ENDIAN,
[
default_sim_xor_endian="ifelse([$1],,8,[$1])"
AC_ARG_ENABLE(sim-xor-endian,
[  --enable-sim-xor-endian=n		Specify number bytes involved in XOR bi-endian mode (default ${default_sim_xor_endian}).],
[case "${enableval}" in
  yes)	sim_xor_endian="-DWITH_XOR_ENDIAN=8";;
  no)	sim_xor_endian="-DWITH_XOR_ENDIAN=0";;
  *)	sim_xor_endian="-DWITH_XOR_ENDIAN=$enableval";;
esac
if test x"$silent" != x"yes" && test x"$sim_xor_endian" != x""; then
  echo "Setting xor-endian flag = $sim_xor_endian" 6>&1
fi],[sim_xor_endian="-DWITH_XOR_ENDIAN=${default_sim_xor_endian}"])dnl
])
AC_SUBST(sim_xor_endian)


dnl --enable-sim-warnings is for developers of the simulator.
dnl it enables extra GCC specific warnings.
AC_DEFUN(SIM_AC_OPTION_WARNINGS,
[
AC_ARG_ENABLE(sim-warnings,
[  --enable-sim-warnings=opts		Extra CFLAGS for turning on compiler warnings],
[case "${enableval}" in
  yes)	sim_warnings="-Werror -Wall -Wpointer-arith -Wmissing-prototypes -Wmissing-declarations ";;
  no)	sim_warnings="-w";;
  *)	sim_warnings=`echo "${enableval}" | sed -e "s/,/ /g"`;;
esac
if test x"$silent" != x"yes" && test x"$sim_warnings" != x""; then
  echo "Setting warning flags = $sim_warnings" 6>&1
fi],[sim_warnings=""])dnl
])
AC_SUBST(sim_warnings)


dnl Generate the Makefile in a target specific directory.
dnl Substitutions aren't performed on the file in AC_SUBST_FILE,
dnl so this is a cover macro to tuck the details away of how we cope.
dnl We cope by having autoconf generate two files and then merge them into
dnl one afterwards.  The two pieces of the common fragment are inserted into
dnl the target's fragment at the appropriate points.

AC_DEFUN(SIM_AC_OUTPUT,
[
AC_LINK_FILES($sim_link_files, $sim_link_links)
AC_OUTPUT(Makefile.sim:Makefile.in Make-common.sim:../common/Make-common.in .gdbinit:../common/gdbinit.in,
[case "x$CONFIG_FILES" in
 xMakefile*)
   echo "Merging Makefile.sim+Make-common.sim into Makefile ..."
   rm -f Makesim1.tmp Makesim2.tmp Makefile
   sed -n -e '/^## COMMON_PRE_/,/^## End COMMON_PRE_/ p' <Make-common.sim >Makesim1.tmp
   sed -n -e '/^## COMMON_POST_/,/^## End COMMON_POST_/ p' <Make-common.sim >Makesim2.tmp
   sed -e '/^## COMMON_PRE_/ r Makesim1.tmp' \
	-e '/^## COMMON_POST_/ r Makesim2.tmp' \
	<Makefile.sim >Makefile
   rm -f Makefile.sim Make-common.sim Makesim1.tmp Makesim2.tmp
   ;;
 esac
 case "x$CONFIG_HEADERS" in xconfig.h:config.in) echo > stamp-h ;; esac
])
])

# Check to see if we're running under Cygwin32, without using
# AC_CANONICAL_*.  If so, set output variable CYGWIN32 to "yes".
# Otherwise set it to "no".

dnl AM_CYGWIN32()
dnl You might think we can do this by checking for a cygwin32-specific
dnl cpp define.
AC_DEFUN(AM_CYGWIN32,
[AC_CACHE_CHECK(for Cygwin32 environment, am_cv_cygwin32,
[AC_TRY_COMPILE(,[int main () { return __CYGWIN32__; }],
am_cv_cygwin32=yes, am_cv_cygwin32=no)
rm -f conftest*])
CYGWIN32=
test "$am_cv_cygwin32" = yes && CYGWIN32=yes])

# Check to see if we're running under Win32, without using
# AC_CANONICAL_*.  If so, set output variable EXEEXT to ".exe".
# Otherwise set it to "".

dnl AM_EXEEXT()
dnl This knows we add .exe if we're building in the Cygwin32
dnl environment. But if we're not, then it compiles a test program
dnl to see if there is a suffix for executables.
AC_DEFUN(AM_EXEEXT,
dnl AC_REQUIRE([AC_PROG_CC])AC_REQUIRE([AM_CYGWIN32])
AC_MSG_CHECKING([for executable suffix])
[AC_CACHE_VAL(am_cv_exeext,
[if test "$CYGWIN32" = yes; then
am_cv_exeext=.exe
else
cat > am_c_test.c << 'EOF'
int main() {
/* Nothing needed here */
}
EOF
${CC-cc} -o am_c_test $CFLAGS $CPPFLAGS $LDFLAGS am_c_test.c $LIBS 1>&5
am_cv_exeext=`ls am_c_test.* | grep -v am_c_test.c | sed -e s/am_c_test//`
rm -f am_c_test*])
test x"${am_cv_exeext}" = x && am_cv_exeext=no
fi
EXEEXT=""
test x"${am_cv_exeext}" != xno && EXEEXT=${am_cv_exeext}
AC_MSG_RESULT(${am_cv_exeext})
AC_SUBST(EXEEXT)])

