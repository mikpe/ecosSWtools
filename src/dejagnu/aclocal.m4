dnl This file is duplicated in four places:
dnl * gdb/aclocal.m4
dnl * gdb/testsuite/aclocal.m4
dnl * expect/aclocal.m4
dnl * dejagnu/aclocal.m4
dnl Consider modifying all copies in parallel.
dnl written by Rob Savoye <rob@cygnus.com> for Cygnus Support
dnl CYGNUS LOCAL: This gets the right posix flag for gcc
AC_DEFUN(CY_AC_TCL_LYNX_POSIX,
[AC_REQUIRE([AC_PROG_CC])AC_REQUIRE([AC_PROG_CPP])
AC_MSG_CHECKING([to see if this is LynxOS])
AC_CACHE_VAL(ac_cv_os_lynx,
[AC_EGREP_CPP(yes,
[/*
 * The old Lynx "cc" only defines "Lynx", but the newer one uses "__Lynx__"
 */
#if defined(__Lynx__) || defined(Lynx)
yes
#endif
], ac_cv_os_lynx=yes, ac_cv_os_lynx=no)])
#
if test "$ac_cv_os_lynx" = "yes" ; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(LYNX)
  AC_MSG_CHECKING([whether -mposix or -X is available])
  AC_CACHE_VAL(ac_cv_c_posix_flag,
  [AC_TRY_COMPILE(,[
  /*
   * This flag varies depending on how old the compiler is.
   * -X is for the old "cc" and "gcc" (based on 1.42).
   * -mposix is for the new gcc (at least 2.5.8).
   */
  #if defined(__GNUC__) && __GNUC__ >= 2
  choke me
  #endif
  ], ac_cv_c_posix_flag=" -mposix", ac_cv_c_posix_flag=" -X")])
  CC="$CC $ac_cv_c_posix_flag"
  AC_MSG_RESULT($ac_cv_c_posix_flag)
  else
  AC_MSG_RESULT(no)
fi
])

#
# Sometimes the native compiler is a bogus stub for gcc or /usr/ucb/cc. This
# makes configure think it's cross compiling. If --target wasn't used, then
# we can't configure, so something is wrong. We don't use the cache
# here cause if somebody fixes their compiler install, we want this to work.
AC_DEFUN(CY_AC_C_WORKS,
[# If we cannot compile and link a trivial program, we can't expect anything to work
AC_MSG_CHECKING(whether the compiler ($CC) actually works)
AC_TRY_COMPILE(, [/* don't need anything here */],
        c_compiles=yes, c_compiles=no)

AC_TRY_LINK(, [/* don't need anything here */],
        c_links=yes, c_links=no)

if test x"${c_compiles}" = x"no" ; then
  AC_MSG_ERROR(the native compiler is broken and won't compile.)
fi

if test x"${c_links}" = x"no" ; then
  AC_MSG_ERROR(the native compiler is broken and won't link.)
fi
AC_MSG_RESULT(yes)
])

AC_DEFUN(CY_AC_PATH_TCLH, [
#
# Ok, lets find the tcl source trees so we can use the headers
# Warning: transition of version 9 to 10 will break this algorithm
# because 10 sorts before 9. We also look for just tcl. We have to
# be careful that we don't match stuff like tclX by accident.
# the alternative search directory is involked by --with-tclinclude
#
no_tcl=true
AC_MSG_CHECKING(for Tcl private headers)
AC_ARG_WITH(tclinclude, [  --with-tclinclude       directory where tcl private headers are], with_tclinclude=${withval})
AC_CACHE_VAL(ac_cv_c_tclh,[
# first check to see if --with-tclinclude was specified
if test x"${with_tclinclude}" != x ; then
  if test -f ${with_tclinclude}/tclInt.h ; then
    ac_cv_c_tclh=`(cd ${with_tclinclude}; pwd)`
  else
    AC_MSG_ERROR([${with_tclinclude} directory doesn't contain private headers])
  fi
fi
# next check in private source directory
#
# since ls returns lowest version numbers first, reverse its output
if test x"${ac_cv_c_tclh}" = x ; then
  for i in \
		${srcdir}/../tcl \
		`ls -dr ${srcdir}/../tcl[[0-9]]* 2>/dev/null` \
		${srcdir}/../../tcl \
		`ls -dr ${srcdir}/../../tcl[[0-9]]* 2>/dev/null` \
		${srcdir}/../../../tcl \
		`ls -dr ${srcdir}/../../../tcl[[0-9]]* 2>/dev/null ` ; do
    if test -f $i/tclInt.h ; then
      ac_cv_c_tclh=`(cd $i; pwd)`
      break
    fi
    # Tcl 7.5 and greater puts headers in subdirectory.
    if test -f $i/generic/tclInt.h ; then
      ac_cv_c_tclh=`(cd $i; pwd)`/generic
      break
    fi
  done
fi
# finally check in a few common install locations
#
# since ls returns lowest version numbers first, reverse its output
if test x"${ac_cv_c_tclh}" = x ; then
  for i in \
		`ls -dr /usr/local/src/tcl[[0-9]]* 2>/dev/null` \
		`ls -dr /usr/local/lib/tcl[[0-9]]* 2>/dev/null` \
		/usr/local/src/tcl \
		/usr/local/lib/tcl \
		${prefix}/include ; do
    if test -f $i/tclInt.h ; then
      ac_cv_c_tclh=`(cd $i; pwd)`
      break
    fi
  done
fi
# see if one is installed
if test x"${ac_cv_c_tclh}" = x ; then
   AC_HEADER_CHECK(tclInt.h, ac_cv_c_tclh=installed, ac_cv_c_tclh="")
fi
])
if test x"${ac_cv_c_tclh}" = x ; then
  TCLHDIR="# no Tcl private headers found"
  AC_MSG_ERROR([Can't find Tcl private headers])
fi
if test x"${ac_cv_c_tclh}" != x ; then
  no_tcl=""
  if test x"${ac_cv_c_tclh}" = x"installed" ; then
    AC_MSG_RESULT([is installed])
    TCLHDIR=""
  else
    AC_MSG_RESULT([found in ${ac_cv_c_tclh}])
    # this hack is cause the TCLHDIR won't print if there is a "-I" in it.
    TCLHDIR="-I${ac_cv_c_tclh}"
  fi
fi

AC_MSG_CHECKING([Tcl version])
orig_includes="$CPPFLAGS"

if test x"${TCLHDIR}" != x ; then
  CPPFLAGS="$CPPFLAGS $TCLHDIR"
fi

# Get major and minor versions of Tcl.  Use funny names to avoid
# clashes with eg SunOS.
cat > conftest.c <<'EOF'
#include "tcl.h"
MaJor = TCL_MAJOR_VERSION
MiNor = TCL_MINOR_VERSION
EOF

tclmajor=
tclminor=
if (eval "$CPP $CPPFLAGS conftest.c") 2>/dev/null >conftest.out; then
   # Success.
   tclmajor=`egrep '^MaJor = ' conftest.out | sed -e 's/^MaJor = *//' -e 's/ *$//'`
   tclminor=`egrep '^MiNor = ' conftest.out | sed -e 's/^MiNor = *//' -e 's/ *$//'`
fi
rm -f conftest.c conftest.out

if test -z "$tclmajor" || test -z "$tclminor"; then
   AC_MSG_RESULT([fatal error: could not find major or minor version number of Tcl])
   exit 1
fi
AC_MSG_RESULT(${tclmajor}.${tclminor})

CPPFLAGS="${orig_includes}"

AC_PROVIDE([$0])
AC_SUBST(TCLHDIR)
])
AC_DEFUN(CY_AC_PATH_TCLLIB, [
#
# Ok, lets find the tcl library
# First, look for one uninstalled.  
# the alternative search directory is invoked by --with-tcllib
#

if test $tclmajor -ge 7 -a $tclminor -ge 4 ; then
  installedtcllibroot=tcl$tclversion
else
  installedtcllibroot=tcl
fi

if test x"${no_tcl}" = x ; then
  # we reset no_tcl incase something fails here
  no_tcl=true
  AC_ARG_WITH(tcllib, [  --with-tcllib           directory where the tcl library is],
         with_tcllib=${withval})
  AC_MSG_CHECKING([for Tcl library])
  AC_CACHE_VAL(ac_cv_c_tcllib,[
  # First check to see if --with-tcllib was specified.
  # This requires checking for both the installed and uninstalled name-styles
  # since we have no idea if it's installed or not.
  if test x"${with_tcllib}" != x ; then
    if test -f "${with_tcllib}/lib$installedtcllibroot.so" ; then
      ac_cv_c_tcllib=`(cd ${with_tcllib}; pwd)`/lib$installedtcllibroot.so
    elif test -f "${with_tcllib}/libtcl.so" ; then
      ac_cv_c_tcllib=`(cd ${with_tcllib}; pwd)`/libtcl.so
    # then look for a freshly built statically linked library
    # if Makefile exists we assume its configured and libtcl will be built first.
    elif test -f "${with_tcllib}/lib$installedtcllibroot.a" ; then
      ac_cv_c_tcllib=`(cd ${with_tcllib}; pwd)`/lib$installedtcllibroot.a
    elif test -f "${with_tcllib}/libtcl.a" ; then
      ac_cv_c_tcllib=`(cd ${with_tcllib}; pwd)`/libtcl.a
    else
      AC_MSG_ERROR([${with_tcllib} directory doesn't contain libraries])
    fi
  fi
  # then check for a private Tcl library
  # Since these are uninstalled, use the simple lib name root.
  if test x"${ac_cv_c_tcllib}" = x ; then
    for i in \
		../tcl \
		`ls -dr ../tcl[[0-9]]* 2>/dev/null` \
		../../tcl \
		`ls -dr ../../tcl[[0-9]]* 2>/dev/null` \
		../../../tcl \
		`ls -dr ../../../tcl[[0-9]]* 2>/dev/null` ; do
      # Tcl 7.5 and greater puts library in subdir.  Look there first.
      if test -f "$i/unix/libtcl.so" ; then
	 ac_cv_c_tcllib=`(cd $i; pwd)`/unix/libtcl.so
	 break
      elif test -f "$i/unix/libtcl.a" -o -f "$i/unix/Makefile"; then
	 ac_cv_c_tcllib=`(cd $i; pwd)`/unix/libtcl.a
	 break
      # look for a freshly built dynamically linked library
      elif test -f "$i/libtcl.so" ; then
        ac_cv_c_tcllib=`(cd $i; pwd)`/libtcl.so
	break

      # then look for a freshly built statically linked library
      # if Makefile exists we assume its configured and libtcl will be
      # built first.
      elif test -f "$i/libtcl.a" -o -f "$i/Makefile" ; then
        ac_cv_c_tcllib=`(cd $i; pwd)`/libtcl.a
	break
      fi
    done
  fi
  # check in a few common install locations
  if test x"${ac_cv_c_tcllib}" = x ; then
    for i in `ls -d ${prefix}/lib /usr/local/lib 2>/dev/null` ; do
      # first look for a freshly built dynamically linked library
      if test -f "$i/lib$installedtcllibroot.so" ; then
        ac_cv_c_tcllib=`(cd $i; pwd)`/lib$installedtcllibroot.so
	break
      # then look for a freshly built statically linked library
      # if Makefile exists we assume its configured and libtcl will be built first.
      elif test -f "$i/lib$installedtcllibroot.a" -o -f "$i/Makefile" ; then
        ac_cv_c_tcllib=`(cd $i; pwd)`/lib$installedtcllibroot.a
	break
      fi
    done
  fi
  # check in a few other private locations
  if test x"${ac_cv_c_tcllib}" = x ; then
    for i in \
		${srcdir}/../tcl \
		`ls -dr ${srcdir}/../tcl[[0-9]]* 2>/dev/null` ; do
      # Tcl 7.5 and greater puts library in subdir.  Look there first.
      if test -f "$i/unix/libtcl.so" ; then
	 ac_cv_c_tcllib=`(cd $i; pwd)`/unix/libtcl.so
	 break
      elif test -f "$i/unix/libtcl.a" -o -f "$i/unix/Makefile"; then
	 ac_cv_c_tcllib=`(cd $i; pwd)`/unix/libtcl.a
	 break
      # look for a freshly built dynamically linked library
      elif test -f "$i/libtcl.so" ; then
        ac_cv_c_tcllib=`(cd $i; pwd)`/libtcl.so
	break

      # then look for a freshly built statically linked library
      # if Makefile exists we assume its configured and libtcl will be
      # built first.
      elif test -f "$i/libtcl.a" -o -f "$i/Makefile" ; then
        ac_cv_c_tcllib=`(cd $i; pwd)`/libtcl.a
	break
      fi
    done
  fi

  # see if one is conveniently installed with the compiler
  if test x"${ac_cv_c_tcllib}" = x ; then	
    orig_libs="$LIBS"
    LIBS="$LIBS -l$installedtcllibroot -lm"    
    AC_TRY_RUN([
    Tcl_AppInit()
    { exit(0); }], ac_cv_c_tcllib="-l$installedtcllibroot", ac_cv_c_tcllib=""
    , ac_cv_c_tclib="-l$installedtcllibroot")
    LIBS="${orig_libs}"
   fi
  ])
  if test x"${ac_cv_c_tcllib}" = x ; then
    TCLLIB="# no Tcl library found"
    AC_MSG_WARN(Can't find Tcl library)
  else
    TCLLIB=${ac_cv_c_tcllib}
    AC_MSG_RESULT(found $TCLLIB)
    no_tcl=
  fi
fi

AC_PROVIDE([$0])
AC_SUBST(TCLLIB)
])
AC_DEFUN(CY_AC_PATH_TKH, [
#
# Ok, lets find the tk source trees so we can use the headers
# If the directory (presumably symlink) named "tk" exists, use that one
# in preference to any others.  Same logic is used when choosing library
# and again with Tcl. The search order is the best place to look first, then in
# decreasing significance. The loop breaks if the trigger file is found.
# Note the gross little conversion here of srcdir by cd'ing to the found
# directory. This converts the path from a relative to an absolute, so
# recursive cache variables for the path will work right. We check all
# the possible paths in one loop rather than many seperate loops to speed
# things up.
# the alternative search directory is invoked by --with-tkinclude
#
AC_MSG_CHECKING(for Tk private headers)
AC_ARG_WITH(tkinclude, [  --with-tkinclude        directory where the tk private headers are],
            with_tkinclude=${withval})
no_tk=true
AC_CACHE_VAL(ac_cv_c_tkh,[
# first check to see if --with-tkinclude was specified
if test x"${with_tkinclude}" != x ; then
  if test -f ${with_tkinclude}/tk.h ; then
    ac_cv_c_tkh=`(cd ${with_tkinclude}; pwd)`
  else
    AC_MSG_ERROR([${with_tkinclude} directory doesn't contain private headers])
  fi
fi
# next check in private source directory
#
# since ls returns lowest version numbers first, reverse the entire list
# and search for the worst fit, overwriting it with better fits as we find them
if test x"${ac_cv_c_tkh}" = x ; then
  for i in \
		${srcdir}/../tk \
		`ls -dr ${srcdir}/../tk[[0-9]]* 2>/dev/null` \
		${srcdir}/../../tk \
		`ls -dr ${srcdir}/../../tk[[0-9]]* 2>/dev/null` \
		${srcdir}/../../../tk \
		`ls -dr ${srcdir}/../../../tk[[0-9]]* 2>/dev/null ` ; do
    if test -f $i/tk.h ; then
      ac_cv_c_tkh=`(cd $i; pwd)`
      break
    fi
    # Tk 4.1 and greater puts this in a subdir.
    if test -f $i/generic/tk.h; then
       ac_cv_c_tkh=`(cd $i; pwd)`/generic
    fi
  done
fi
# finally check in a few common install locations
#
# since ls returns lowest version numbers first, reverse the entire list
# and search for the worst fit, overwriting it with better fits as we find them
if test x"${ac_cv_c_tkh}" = x ; then
  for i in \
		`ls -dr /usr/local/src/tk[[0-9]]* 2>/dev/null` \
		`ls -dr /usr/local/lib/tk[[0-9]]* 2>/dev/null` \
		/usr/local/src/tk \
		/usr/local/lib/tk \
		${prefix}/include ; do
    if test -f $i/tk.h ; then
      ac_cv_c_tkh=`(cd $i; pwd)`
      break
    fi
  done
fi
# see if one is installed
if test x"${ac_cv_c_tkh}" = x ; then
  AC_HEADER_CHECK(tk.h, ac_cv_c_tkh=installed)
fi
])
if test x"${ac_cv_c_tkh}" != x ; then
  no_tk=""
  if test x"${ac_cv_c_tkh}" = x"installed" ; then
    AC_MSG_RESULT([is installed])
    TKHDIR=""
  else
    AC_MSG_RESULT([found in $ac_cv_c_tkh])
    # this hack is cause the TKHDIR won't print if there is a "-I" in it.
    TKHDIR="-I${ac_cv_c_tkh}"
  fi
else
  TKHDIR="# no Tk directory found"
  AC_MSG_WARN([Can't find Tk private headers])
  no_tk=true
fi

# if Tk is installed, extract the major/minor version
if test x"${no_tk}" = x ; then
AC_MSG_CHECKING([Tk version])
orig_includes="$CPPFLAGS"

if test x"${TCLHDIR}" != x ; then
  CPPFLAGS="$CPPFLAGS $TCLHDIR"
fi
if test x"${TKHDIR}" != x ; then
  CPPFLAGS="$CPPFLAGS $TKHDIR"
fi
if test x"${x_includes}" != x -a x"${x_includes}" != xNONE ; then
  CPPFLAGS="$CPPFLAGS -I$x_includes"
fi

# Get major and minor versions of Tk.  Use funny names to avoid
# clashes with eg SunOS.
cat > conftest.c <<'EOF'
#include "tk.h"
MaJor = TK_MAJOR_VERSION
MiNor = TK_MINOR_VERSION
EOF

tkmajor=
tkminor=
if (eval "$CPP $CPPFLAGS conftest.c") 2>/dev/null >conftest.out; then
   # Success.
   tkmajor=`egrep '^MaJor = ' conftest.out | sed -e 's/^MaJor = *//' -e 's/ *$//'`
   tkminor=`egrep '^MiNor = ' conftest.out | sed -e 's/^MiNor = *//' -e 's/ *$//'`
fi
rm -f conftest.c conftest.out

if test -z "$tkmajor" || test -z "$tkminor"; then
   AC_MSG_RESULT([fatal error: could not find major or minor version number of Tk])
   exit 1
fi
AC_MSG_RESULT(${tkmajor}.${tkminor})

CPPFLAGS="${orig_includes}"
fi

AC_PROVIDE([$0])
AC_SUBST(TKHDIR)
])
AC_DEFUN(CY_AC_PATH_TKLIB, [
AC_REQUIRE([CY_AC_PATH_TCL])
#
# Ok, lets find the tk library
# First, look for the latest private (uninstalled) copy
# Notice that the destinations in backwards priority since the tests have
# no break.
# Then we look for either .a, .so, or Makefile.  A Makefile is acceptable
# is it indicates the target has been configured and will (probably)
# soon be built.  This allows an entire tree of Tcl software to be
# configured at once and then built.
# the alternative search directory is invoked by --with-tklib
#

if test x"${no_tk}" = x ; then
  # reset no_tk incase something fails here
  no_tk="true"

  if test $tkmajor -ge 4 ; then
    installedtklibroot=tk$tkversion
  else
    installedtklibroot=tk
  fi

  AC_ARG_WITH(tklib, [  --with-tklib            directory where the tk library is],
              with_tklib=${withval})
  AC_MSG_CHECKING([for Tk library])
  AC_CACHE_VAL(ac_cv_c_tklib,[
  # first check to see if --with-tklib was specified
  # This requires checking for both the installed and uninstalled name-styles
  # since we have no idea if it's installed or not.
  if test x"${with_tklib}" != x ; then
    if test -f "${with_tklib}/lib$installedtklibroot.so" ; then
      ac_cv_c_tklib=`(cd ${with_tklib}; pwd)`/lib$installedtklibroot.so
      no_tk=""
    elif test -f "${with_tklib}/libtk.so" ; then
      ac_cv_c_tklib=`(cd ${with_tklib}; pwd)`/libtk.so
      no_tk=""
    # then look for a freshly built statically linked library
    # if Makefile exists we assume its configured and libtk will be built
    elif test -f "${with_tklib}/lib$installedtklibroot.a" ; then
      ac_cv_c_tklib=`(cd ${with_tklib}; pwd)`/lib$installedtklibroot.a
      no_tk=""
    elif test -f "${with_tklib}/libtk.a" ; then
      ac_cv_c_tklib=`(cd ${with_tklib}; pwd)`/libtk.a
      no_tk=""
    else
      AC_MSG_ERROR([${with_tklib} directory doesn't contain libraries])
    fi
  fi
  # then check for a private Tk library
  # Since these are uninstalled, use the simple lib name root.
  if test x"${ac_cv_c_tklib}" = x ; then
    for i in \
		../tk \
		`ls -dr ../tk[[0-9]]* 2>/dev/null` \
		../../tk \
		`ls -dr ../../tk[[0-9]]* 2>/dev/null` \
		../../../tk \
		`ls -dr ../../../tk[[0-9]]* 2>/dev/null` ; do
      # Tk 4.1 and greater puts things in subdirs.  Check these first.
      if test -f "$i/unix/libtk.so" ; then
	 ac_cv_c_tklib=`(cd $i; pwd)`/unix/libtk.so
	 no_tk=
	 break
      elif test -f "$i/unix/libtk.a" -o -f "$i/unix/Makefile"; then
	 ac_cv_c_tklib=`(cd $i; pwd)`/unix/libtk.a
	 no_tk=
	 break
      # look for a freshly built dynamically linked library
      elif test -f "$i/libtk.so" ; then
        ac_cv_c_tklib=`(cd $i; pwd)`/libtk.so
        no_tk=
	break
      # then look for a freshly built statically linked library
      # if Makefile exists we assume its configured and libtk will be built 
      elif test -f "$i/libtk.a" -o -f "$i/Makefile" ; then
        ac_cv_c_tklib=`(cd $i; pwd)`/libtk.a
        no_tk=""
 	break
      fi
    done
  fi
  # finally check in a few common install locations
  if test x"${ac_cv_c_tklib}" = x ; then
    for i in `ls -d ${prefix}/lib /usr/local/lib 2>/dev/null` ; do
      # first look for a freshly built dynamically linked library
      if test -f "$i/lib$installedtklibroot.so" ; then
        ac_cv_c_tklib=`(cd $i; pwd)`/lib$installedtklibroot.so
        no_tk=""
	break
      # then look for a freshly built statically linked library
      # if Makefile exists, we assume it's configured and libtcl will be built 
      elif test -f "$i/lib$installedtklibroot.a" -o -f "$i/Makefile" ; then
        ac_cv_c_tklib=`(cd $i; pwd)`/lib$installedtklibroot.a
        no_tk=""
 	break
      fi
    done
  fi
  # check in a few other private locations
  if test x"${ac_cv_c_tklib}" = x ; then
    for i in \
		${srcdir}/../tk \
		`ls -dr ${srcdir}/../tk[[0-9]]* 2>/dev/null` ; do
      # Tk 4.1 and greater puts things in subdirs.  Check these first.
      if test -f "$i/unix/libtk.so" ; then
	 ac_cv_c_tklib=`(cd $i; pwd)`/unix/libtk.so
	 no_tk=
	 break
      elif test -f "$i/unix/libtk.a" -o -f "$i/unix/Makefile"; then
	 ac_cv_c_tcllib=`(cd $i; pwd)`/unix/libtk.a
	 no_tk=
	 break
      # look for a freshly built dynamically linked library
      elif test -f "$i/libtk.so" ; then
        ac_cv_c_tklib=`(cd $i; pwd)`/libtk.so
        no_tk=""
	break
      # then look for a freshly built statically linked library
      # if Makefile exists, we assume it's configured and libtcl will be built 
      elif test -f "$i/libtk.a" -o -f "$i/Makefile" ; then
        ac_cv_c_tklib=`(cd $i; pwd)`/libtk.a
        no_tk=""
 	break
      fi
    done
  fi
  # see if one is conveniently installed with the compiler
  if test x"${ac_cv_c_tklib}" = x ; then
       AC_REQUIRE([AC_PATH_X])
       orig_libs="$LIBS"
       LIBS="$LIBS -l$installedtklibroot $x_libraries $ac_cv_c_tcllib -lm"    
       AC_TRY_RUN([
       Tcl_AppInit()
       { exit(0); }], ac_cv_c_tklib="-l$installedtklibroot", ac_cv_c_tklib=""
       , ac_cv_c_tklib="-l$installedtklibroot")
       LIBS="${orig_libs}"
   fi
  ])
  if test x"${ac_cv_c_tklib}" = x ; then
    TKLIB="# no Tk library found"
    AC_MSG_WARN(Can't find Tk library)
  else
    TKLIB=$ac_cv_c_tklib
    AC_MSG_RESULT(found $TKLIB)
    no_tk=
  fi
fi
AC_PROVIDE([$0])
AC_SUBST(TKLIB)
])
AC_DEFUN(CY_AC_PATH_TK, [
  CY_AC_PATH_TKH
  CY_AC_PATH_TKLIB
])
AC_DEFUN(CY_AC_PATH_TCL, [
  CY_AC_PATH_TCLH
  CY_AC_PATH_TCLLIB
])
