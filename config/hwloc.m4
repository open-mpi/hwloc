dnl -*- Autoconf -*-
dnl
dnl Copyright 2009 INRIA, Université Bordeaux 1
dnl Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
dnl                         University Research and Technology
dnl                         Corporation.  All rights reserved.
dnl Copyright (c) 2004-2005 The Regents of the University of California.
dnl                         All rights reserved.
dnl Copyright (c) 2004-2008 High Performance Computing Center Stuttgart, 
dnl                         University of Stuttgart.  All rights reserved.
dnl Copyright © 2006-2010  Cisco Systems, Inc.  All rights reserved.

#-----------------------------------------------------------------------

# Main hwloc m4 macro, to be invoked by the user
#
# Expects two or three paramters:
# 1. Configuration prefix
# 2. What to do upon success
# 3. What to do upon failure
#
AC_DEFUN([HWLOC_INIT],[
    AC_REQUIRE([AC_CANONICAL_TARGET])
    AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])
    AC_REQUIRE([_HWLOC_DEFINE_ARGS])
    AC_REQUIRE([AC_PROG_CC])
    AC_REQUIRE([AM_PROG_CC_C_O])
    AC_REQUIRE([AC_PROG_CC_C99])

    m4_ifval([$1], 
             [m4_define([hwloc_config_prefix],[$1/])],
             [m4_define([hwloc_config_prefix], [])])

    # Get hwloc's absolute top builddir (which may not be the same as
    # the real $top_builddir, because we may be building in embedded
    # mode).
    HWLOC_startdir=`pwd`
    if test x"hwloc_config_prefix" != "x" -a ! -d "hwloc_config_prefix"; then
        mkdir "hwloc_config_prefix"
    fi
    cd "hwloc_config_prefix"
    HWLOC_top_builddir=`pwd`
    AC_SUBST(HWLOC_top_builddir)

    # Get hwloc's absolute top srcdir (which may not be the same as
    # the real $top_srcdir, because we may be building in embedded
    # mode).  First, go back to the startdir incase the $srcdir is
    # relative.
    cd "$HWLOC_startdir"
    cd "$srcdir"/hwloc_config_prefix
    HWLOC_top_srcdir="`pwd`"
    AC_SUBST(HWLOC_top_srcdir)

    # Go back to where we started
    cd "$HWLOC_startdir"

    AC_MSG_NOTICE([hwloc builddir: $HWLOC_top_builddir])
    AC_MSG_NOTICE([hwloc srcdir: $HWLOC_top_srcdir])
    if test "$HWLOC_top_builddir" != "$HWLOC_top_srcdir"; then
        AC_MSG_NOTICE([Detected VPATH build])
    fi

    # Are we building as standalone or embedded?
    AC_MSG_CHECKING([for hwloc building mode])
    AC_MSG_RESULT([$hwloc_mode])

    # Debug mode?
    AC_MSG_CHECKING([if want hwloc maintainer support])
    hwloc_debug=
    AS_IF([test "$enable_debug" = "yes"],
          [hwloc_debug=1
           hwloc_debug_msg="enabled"])
    AS_IF([test "$hwloc_debug" = "" -a "$hwloc_mode" = "embedded" -a "$enable_debug" = ""],
          [hwloc_debug=0
           hwloc_debug_msg="disabled (embedded mode)"])
    AS_IF([test "$hwloc_debug" = "" -a "$enable_debug" = "" -a -d .svn],
          [hwloc_debug=1
           hwloc_debug_msg="enabled (SVN checkout default)"])
    AS_IF([test "$hwloc_debug" = "" -a "$enable_debug" = "" -a -d .hg],
          [hwloc_debug=1
           hwloc_debug_msg="enabled (HG clone default)"])
    AS_IF([test "$hwloc_debug" = ""],
          [hwloc_debug=0
           hwloc_debug_msg="disabled"])
    # Grr; we use #ifndef for HWLOC_DEBUG!  :-(
    AH_TEMPLATE(HWLOC_DEBUG, [Whether we are in debugging mode or not])
    AS_IF([test "$hwloc_debug" = "1"], [AC_DEFINE([HWLOC_DEBUG])])
    AC_MSG_RESULT([$hwloc_debug_msg])

    # We need to set a path for header, etc files depending on whether
    # we're standalone or embedded. this is taken care of by HWLOC_EMBEDDED.

    AC_MSG_CHECKING([for hwloc directory prefix])
    AC_MSG_RESULT(m4_ifval([$1], hwloc_config_prefix, [(none)]))

    # Note that private/config.h *MUST* be listed first so that it
    # becomes the "main" config header file.  Any AM_CONFIG_HEADERs
    # after that (hwloc/config.h) will only have selective #defines
    # replaced, not the entire file.
    AM_CONFIG_HEADER(hwloc_config_prefix[include/private/config.h])
    AM_CONFIG_HEADER(hwloc_config_prefix[include/hwloc/config.h])

    # What prefix are we using?
    AC_MSG_CHECKING([for hwloc symbol prefix])
    AS_IF([test "$hwloc_symbol_prefix_value" = ""],
          [AS_IF([test "$with_hwloc_symbol_prefix" = ""],
                 [hwloc_symbol_prefix_value=hwloc_],
                 [hwloc_symbol_prefix_value=$with_hwloc_symbol_prefix])])
    AC_DEFINE_UNQUOTED(HWLOC_SYM_PREFIX, [$hwloc_symbol_prefix_value],
                       [The hwloc symbol prefix])
    # Ensure to [] escape the whole next line so that we can get the
    # proper tr tokens
    [hwloc_symbol_prefix_value_caps="`echo $hwloc_symbol_prefix_value | tr '[:lower:]' '[:upper:]'`"]
    AC_DEFINE_UNQUOTED(HWLOC_SYM_PREFIX_CAPS, [$hwloc_symbol_prefix_value_caps],
                       [The hwloc symbol prefix in all caps])
    AC_MSG_RESULT([$hwloc_symbol_prefix_value])

    # Give an easy #define to know if we need to transform all the
    # hwloc names
    AH_TEMPLATE([HWLOC_SYM_TRANSFORM], [Whether we need to re-define all the hwloc public symbols or not])
    AS_IF([test "$hwloc_symbol_prefix_value" = "hwloc_"],
          [AC_DEFINE([HWLOC_SYM_TRANSFORM], [0])],
          [AC_DEFINE([HWLOC_SYM_TRANSFORM], [1])])

    #
    # Setup hwloc docs support if we're building standalone
    #

    AS_IF([test "$hwloc_mode" = "standalone"], 
          [_HWLOC_SETUP_DOCS],
          [hwloc_generate_doxs=no
           hwloc_generate_readme=no
           hwloc_install_doxs=no])

    #
    # Check OS support
    #
    AC_MSG_CHECKING([which OS support to include])
    case ${target} in
      *-*-linux*)
        AC_DEFINE(HWLOC_LINUX_SYS, 1, [Define to 1 on Linux])
        hwloc_linux=yes
        AC_MSG_RESULT([Linux])
        ;;
      *-*-irix*)
        AC_DEFINE(HWLOC_IRIX_SYS, 1, [Define to 1 on Irix])
        hwloc_irix=yes
        AC_MSG_RESULT([IRIX])
        ;;
      *-*-darwin*)
        AC_DEFINE(HWLOC_DARWIN_SYS, 1, [Define to 1 on Darwin])
        hwloc_darwin=yes
        AC_MSG_RESULT([Darwin])
        ;;
      *-*-solaris*)
        AC_DEFINE(HWLOC_SOLARIS_SYS, 1, [Define to 1 on Solaris])
        hwloc_solaris=yes
        AC_MSG_RESULT([Solaris])
        ;;
      *-*-aix*)
        AC_DEFINE(HWLOC_AIX_SYS, 1, [Define to 1 on AIX])
        hwloc_aix=yes
        AC_MSG_RESULT([AIX])
        ;;
      *-*-osf*)
        AC_DEFINE(HWLOC_OSF_SYS, 1, [Define to 1 on OSF])
        hwloc_osf=yes
        AC_MSG_RESULT([OSF])
        ;;
      *-*-hpux*)
        AC_DEFINE(HWLOC_HPUX_SYS, 1, [Define to 1 on HP-UX])
        hwloc_hpux=yes
        AC_MSG_RESULT([HP-UX])
        ;;
      *-*-mingw*|*-*-cygwin*)
        AC_DEFINE(HWLOC_WIN_SYS, 1, [Define to 1 on WINDOWS])
        hwloc_windows=yes
        AC_MSG_RESULT([Windows])
        ;;
      *-*-*freebsd*)
        AC_DEFINE(HWLOC_FREEBSD_SYS, 1, [Define to 1 on *FREEBSD])
        hwloc_freebsd=yes
        AC_MSG_RESULT([FreeBSD])
        ;;
      *)
        AC_MSG_RESULT([Unsupported! ($target)])
        AC_DEFINE(HWLOC_UNSUPPORTED_SYS, 1, [Define to 1 on unsupported systems])
        AC_MSG_WARN([***********************************************************])
        AC_MSG_WARN([*** hwloc does not support this system.])
        AC_MSG_WARN([*** hwloc will *attempt* to build (but it may not work).])
        AC_MSG_WARN([*** hwloc's run-time results may be reduced to showing just one processor.])
        AC_MSG_WARN([*** You have been warned.])
        AC_MSG_WARN([*** Pausing to give you time to read this message...])
        AC_MSG_WARN([***********************************************************])
        sleep 10
        ;;
    esac

    #
    # Check CPU support
    #
    AC_MSG_CHECKING([which CPU support to include])
    case ${target} in
      i*86-*-*)
        AC_DEFINE(HWLOC_X86_32_ARCH, 1, [Define to 1 on x86_32])
        hwloc_x86_32=yes
        AC_MSG_RESULT([x86_32])
        ;;
      x86_64-*-*)
        AC_DEFINE(HWLOC_X86_64_ARCH, 1, [Define to 1 on x86_64])
        hwloc_x86_64=yes
        AC_MSG_RESULT([x86_64])
        ;;
    esac
    
    #
    # Define C flags
    #

    AC_USE_SYSTEM_EXTENSIONS # for O_DIRECTORY, fdopen, ffsl, ...
    AH_VERBATIM([USE_HPUX_SYSTEM_EXTENSIONS],
[/* Enable extensions on HP-UX. */
#ifndef _HPUX_SOURCE
# undef _HPUX_SOURCE
#endif
])
    AC_DEFINE([_HPUX_SOURCE], [1], [Are we building for HP-UX?])
    
    AC_LANG_PUSH([C])
    
    _HWLOC_GCC_FLAGS
    _HWLOC_CHECK_DIFF_U
    
    AC_CHECK_SIZEOF([unsigned long])
    AC_DEFINE_UNQUOTED([HWLOC_SIZEOF_UNSIGNED_LONG], $ac_cv_sizeof_unsigned_long, [The size of `unsigned long', as computed by sizeof])
    AC_CHECK_SIZEOF([unsigned int])
    AC_DEFINE_UNQUOTED([HWLOC_SIZEOF_UNSIGNED_INT], $ac_cv_sizeof_unsigned_int, [The size of `unsigned int', as computed by sizeof])

    #
    # Check for compiler attributes and visibility
    #
    _HWLOC_CHECK_ATTRIBUTES
    _HWLOC_CHECK_VISIBILITY
    
    #
    # Now detect support
    #
    
    hwloc_strncasecmp=strncmp
    AC_CHECK_FUNCS([strncasecmp], [
      _HWLOC_CHECK_DECL([strncasecmp], [
        hwloc_strncasecmp=strncasecmp
      ])
    ])
    AC_DEFINE_UNQUOTED(hwloc_strncasecmp, $hwloc_strncasecmp, [Define this to either strncasecmp or strncmp])
    
    AC_CHECK_TYPES([wchar_t], [
      AC_CHECK_FUNCS([putwc])
    ], [], [[#include <wchar.h>]])
    
    AC_CHECK_HEADERS([locale.h], [
      AC_CHECK_FUNCS([setlocale])
    ])
    AC_CHECK_HEADERS([langinfo.h], [
      AC_CHECK_FUNCS([nl_langinfo])
    ])
    old_LIBS="$LIBS"
    LIBS=
    AC_CHECK_HEADERS([curses.h], [
      AC_CHECK_HEADERS([term.h], [
        AC_SEARCH_LIBS([tparm], [termcap curses], [
            AC_SUBST([HWLOC_TERMCAP_LIBS], ["$LIBS"])
            AC_DEFINE([HWLOC_HAVE_LIBTERMCAP], [1],
                      [Define to 1 if you have a library providing the termcap interface])
          ])
      ], [], [[#include <curses.h>]])
    ])
    LIBS="$old_LIBS"

    AC_CHECK_TYPES([KAFFINITY,
                    PROCESSOR_CACHE_TYPE,
                    CACHE_DESCRIPTOR,
                    LOGICAL_PROCESSOR_RELATIONSHIP,
                    SYSTEM_LOGICAL_PROCESSOR_INFORMATION,
                    GROUP_AFFINITY,
                    PROCESSOR_RELATIONSHIP,
                    NUMA_NODE_RELATIONSHIP,
                    CACHE_RELATIONSHIP,
                    PROCESSOR_GROUP_INFO,
                    GROUP_RELATIONSHIP,
                    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX],
                    [],[],[[#include <windows.h>]])
    AC_HAVE_LIBRARY(gdi32)
    
    AC_CHECK_HEADER([windows.h], [
      AC_DEFINE([HWLOC_HAVE_WINDOWS_H], [1], [Define to 1 if you have the `windows.h' header.])
    ])
    
    AC_CHECK_HEADERS([sys/lgrp_user.h], [
      AC_HAVE_LIBRARY([lgrp])
    ])
    AC_CHECK_HEADERS([kstat.h], [
      AC_HAVE_LIBRARY([kstat])
    ])
    
    AC_CHECK_HEADERS([infiniband/verbs.h], [
      AC_HAVE_LIBRARY([ibverbs], [hwloc_have_libibverbs=yes])
    ])
    
    AC_CHECK_DECLS([_SC_NPROCESSORS_ONLN,
    		_SC_NPROCESSORS_CONF,
    		_SC_NPROC_ONLN,
    		_SC_NPROC_CONF,
    		_SC_LARGE_PAGESIZE],,[:],[[#include <unistd.h>]])
    
    AC_HAVE_HEADERS([mach/mach_host.h])
    AC_HAVE_HEADERS([mach/mach_init.h], [
      AC_CHECK_FUNCS([host_info])
    ])

    AC_CHECK_HEADERS([sys/param.h])
    AC_CHECK_HEADERS([sys/sysctl.h], [
      AC_CHECK_DECLS([CTL_HW, HW_NCPU],,,[[
      #if HAVE_SYS_PARAM_H
      #include <sys/param.h>
      #endif
      #include <sys/sysctl.h>
      ]])
    ],,[
      AC_INCLUDES_DEFAULT
      #if HAVE_SYS_PARAM_H
      #include <sys/param.h>
      #endif
    ])
    AC_CHECK_FUNCS([sysctl sysctlbyname])

    case ${target} in
      *-*-mingw*|*-*-cygwin*)
        hwloc_pid_t=HANDLE
        hwloc_thread_t=HANDLE
        ;;
      *)
        hwloc_pid_t=pid_t
        AC_CHECK_TYPES([pthread_t], [hwloc_thread_t=pthread_t], [:], [[#include <pthread.h>]])
        ;;
    esac
    AC_DEFINE_UNQUOTED(hwloc_pid_t, $hwloc_pid_t, [Define this to the process ID type])
    if test "x$hwloc_thread_t" != "x" ; then
      AC_DEFINE_UNQUOTED(hwloc_thread_t, $hwloc_thread_t, [Define this to the thread ID type])
    fi
    
    _HWLOC_CHECK_DECL([sched_setaffinity], [
      AC_MSG_CHECKING([for old prototype of sched_setaffinity])
      AC_COMPILE_IFELSE(
        AC_LANG_PROGRAM([[
          #define _GNU_SOURCE
          #include <sched.h>
          static unsigned long mask;
          ]], [[ sched_setaffinity(0, (void*) &mask);
          ]]),
        AC_DEFINE([HWLOC_HAVE_OLD_SCHED_SETAFFINITY], [1], [Define to 1 if glibc provides the old prototype of sched_setaffinity()])
        AC_MSG_RESULT([yes]),
        AC_MSG_RESULT([no])
      )
    ])
    
    AC_MSG_CHECKING([for working CPU_SET])
    AC_LINK_IFELSE(
      AC_LANG_PROGRAM([[
        #include <sched.h>
        cpu_set_t set;
        ]], [[ CPU_ZERO(&set); CPU_SET(0, &set);
        ]]),
        AC_DEFINE([HWLOC_HAVE_CPU_SET], [1], [Define to 1 if the CPU_SET macro works])
        AC_MSG_RESULT([yes]),
        AC_MSG_RESULT([no])
    )
    
    AC_MSG_CHECKING([for working CPU_SET_S])
    AC_LINK_IFELSE(
      AC_LANG_PROGRAM([[
          #include <sched.h>
          cpu_set_t *set;
        ]], [[
          set = CPU_ALLOC(1024);
          CPU_ZERO_S(CPU_ALLOC_SIZE(1024), set);
          CPU_SET_S(CPU_ALLOC_SIZE(1024), 0, set);
          CPU_FREE(set);
        ]]),
        AC_DEFINE([HWLOC_HAVE_CPU_SET_S], [1], [Define to 1 if the CPU_SET_S macro works])
        AC_MSG_RESULT([yes]),
        AC_MSG_RESULT([no])
    )

    # Cairo support
    if test "x$enable_cairo" != "xno"; then
      HWLOC_PKG_CHECK_MODULES([CAIRO], [cairo], [:], [enable_cairo="no"])
      if test "x$enable_cairo" != "xno"; then
        AC_PATH_XTRA
	CFLAGS_save=$CFLAGS
	LIBS_save=$LIBS

	CFLAGS="$CFLAGS $X_CFLAGS"
	LIBS="$LIBS $X_PRE_LIBS $X_LIBS $X_EXTRA_LIBS"
        AC_CHECK_HEADERS([X11/Xlib.h], [
          AC_CHECK_HEADERS([X11/Xutil.h X11/keysym.h], [
            AC_CHECK_LIB([X11], [XOpenDisplay], [
              enable_X11=yes
              AC_SUBST([HWLOC_X11_LIBS], ["-lX11"])
              AC_DEFINE([HWLOC_HAVE_X11], [1], [Define to 1 if X11 libraries are available.])
            ])]
          )],,
          [[#include <X11/Xlib.h>]]
        )
        if test "x$enable_X11" != "xyes"; then
          AC_MSG_WARN([X11 headers not found, Cairo/X11 back-end disabled])
        fi

	CFLAGS=$CFLAGS_save
	LIBS=$LIBS_save
      fi
    fi
    
    if test "x$enable_cairo" != "xno"; then
      AC_DEFINE([HWLOC_HAVE_CAIRO], [1], [Define to 1 if you have the `cairo' library.])
    fi

    # XML support        
    
    if test "x$enable_xml" != "xno"; then
      HWLOC_PKG_CHECK_MODULES([XML], [libxml-2.0], [:], [enable_xml="no"])
    fi
    
    if test "x$enable_xml" != "xno"; then
      HWLOC_REQUIRES="libxml-2.0 $HWLOC_REQUIRES"
      AC_DEFINE([HWLOC_HAVE_XML], [1], [Define to 1 if you have the `xml' library.])
      AC_SUBST([HWLOC_HAVE_XML], [1])
    else
      AC_SUBST([HWLOC_HAVE_XML], [0])
    fi
    
    # check for kerrighed, but don't abort if not found
    HWLOC_PKG_CHECK_MODULES([KERRIGHED], [kerrighed >= 2.0], [], [:])
    
    # disable C++, F77, Java and Windows Resource Compiler support
    LT_PREREQ([2.2.6])
    LT_INIT
    LT_LANG([C])
    AC_LIBTOOL_WIN32_DLL
    AC_PATH_PROGS([HWLOC_MS_LIB], [lib])
    AC_ARG_VAR([HWLOC_MS_LIB], [Path to Microsoft's Visual Studio `lib' tool])

    AC_PATH_PROG([BASH], [bash])
    
    AC_CHECK_FUNCS([ffs], [
      _HWLOC_CHECK_DECL([ffs],[
        AC_DEFINE([HWLOC_HAVE_DECL_FFS], [1], [Define to 1 if function `ffs' is declared by system headers])
      ])
      AC_DEFINE([HWLOC_HAVE_FFS], [1], [Define to 1 if you have the `ffs' function.])
    ])
    AC_CHECK_FUNCS([ffsl], [
      _HWLOC_CHECK_DECL([ffsl],[
        AC_DEFINE([HWLOC_HAVE_DECL_FFSL], [1], [Define to 1 if function `ffsl' is declared by system headers])
      ])
      AC_DEFINE([HWLOC_HAVE_FFSL], [1], [Define to 1 if you have the `ffsl' function.])
    ])
    
    AC_CHECK_FUNCS([fls], [
      _HWLOC_CHECK_DECL([fls],[
        AC_DEFINE([HWLOC_HAVE_DECL_FLS], [1], [Define to 1 if function `fls' is declared by system headers])
      ])
      AC_DEFINE([HWLOC_HAVE_FLS], [1], [Define to 1 if you have the `fls' function.])
    ])
    AC_CHECK_FUNCS([flsl], [
      _HWLOC_CHECK_DECL([flsl],[
        AC_DEFINE([HWLOC_HAVE_DECL_FLSL], [1], [Define to 1 if function `flsl' is declared by system headers])
      ])
      AC_DEFINE([HWLOC_HAVE_FLSL], [1], [Define to 1 if you have the `flsl' function.])
    ])
    
    AC_CHECK_FUNCS([clz], [
      _HWLOC_CHECK_DECL([clz],[
        AC_DEFINE([HWLOC_HAVE_DECL_CLZ], [1], [Define to 1 if function `clz' is declared by system headers])
      ])
      AC_DEFINE([HWLOC_HAVE_CLZ], [1], [Define to 1 if you have the `clz' function.])
    ])
    AC_CHECK_FUNCS([clzl], [
      _HWLOC_CHECK_DECL([clzl],[
        AC_DEFINE([HWLOC_HAVE_DECL_CLZL], [1], [Define to 1 if function `clzl' is declared by system headers])
      ])
      AC_DEFINE([HWLOC_HAVE_CLZL], [1], [Define to 1 if you have the `clzl' function.])
    ])
    
    AC_CHECK_FUNCS([openat], [have_openat=yes])
    
    AC_CHECK_DECL([numa_bitmask_alloc], [have_linux_libnuma=yes], [],
    	      [#include <numa.h>])
    
    AC_CHECK_HEADERS([pthread_np.h])
    AC_CHECK_DECLS([pthread_setaffinity_np],,[:],[[
      #include <pthread.h>
      #ifdef HAVE_PTHREAD_NP_H
      #  include <pthread_np.h>
      #endif
    ]])
    AC_CHECK_DECLS([pthread_getaffinity_np],,[:],[[
      #include <pthread.h>
      #ifdef HAVE_PTHREAD_NP_H
      #  include <pthread_np.h>
      #endif
    ]])
    AC_CHECK_FUNC([sched_setaffinity], [have_sched_setaffinity=yes])
    AC_CHECK_HEADERS([sys/cpuset.h],,,[[#include <sys/param.h>]])
    
    AC_CHECK_PROGS(XMLLINT, [xmllint])
    
    AC_SUBST(HWLOC_REQUIRES)

    # Setup HWLOC's CPP and LD flags
    HWLOC_CPPFLAGS='-I$(HWLOC_top_srcdir)/include -I$(HWLOC_top_builddir)/include'
    AC_SUBST(HWLOC_CPPFLAGS)
    HWLOC_LDFLAGS='-L$(HWLOC_top_builddir)/src'
    AC_SUBST(HWLOC_LDFLAGS)

    # Set these values explicitly for embedded builds.  Exporting
    # these values through *_EMBEDDED_* values gives us the freedom to
    # do something different someday if we ever need to.
    HWLOC_EMBEDDED_CPPFLAGS=$HWLOC_CPPFLAGS
    AC_SUBST(HWLOC_EMBEDDED_CPPFLAGS)
    HWLOC_EMBEDDED_LDADD='$(HWLOC_top_builddir)/src/libhwloc_embedded.la'
    AC_SUBST(HWLOC_EMBEDDED_LDADD)

    # Try to compile the cpuid inlines
    AC_MSG_CHECKING([for cpuid])
    old_CPPFLAGS="$CPPFLAGS"
    CFLAGS="$CFLAGS -I$HWLOC_top_srcdir/include"
    AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[
        #include <stdio.h>
        #include <private/cpuid.h>
      ]], [[
        if (hwloc_have_cpuid()) {
          unsigned eax = 0, ebx, ecx = 0, edx;
          hwloc_cpuid(&eax, &ebx, &ecx, &edx);
          printf("highest cpuid %x\n", eax);
          return 0;
        }
      ]]), [
      AC_MSG_RESULT([yes])
      AC_DEFINE(HWLOC_HAVE_CPUID, 1, [Define to 1 if you have cpuid])
      hwloc_have_cpuid=yes
    ], [
      AC_MSG_RESULT([no])
    ])
    CPPFLAGS="$old_CPPFLAGS"
    # Setup all the AM_CONDITIONALs
    HWLOC_DO_AM_CONDITIONALS

    # Only generate these files/links in standalone mode
    AS_IF([test "$hwloc_mode" = "standalone"],
          [
           # These files are only needed in standalone mode
           AC_CONFIG_FILES(
               hwloc_config_prefix[hwloc.pc]
               hwloc_config_prefix[doc/doxygen-config.cfg]
               hwloc_config_prefix[tests/linux/gather-topology.sh]
               hwloc_config_prefix[tests/linux/test-topology.sh]
               hwloc_config_prefix[tests/xml/test-topology.sh]
               hwloc_config_prefix[utils/test-hwloc-distrib.sh])

           AC_CONFIG_COMMANDS([chmoding-scripts], [chmod +x ]hwloc_config_prefix[tests/linux/test-topology.sh ]hwloc_config_prefix[tests/xml/test-topology.sh ]hwloc_config_prefix[tests/linux/gather-topology.sh ]hwloc_config_prefix[utils/test-hwloc-distrib.sh])

           # These links are only needed in standalone mode.  It would
           # be nice to m4 foreach this somehow, but whenever I tried
           # it, I got obscure "invalid tag" errors from
           # AC_CONFIG_LINKS.  :-\ Since these tests are only run when
           # built in standalone mode, only generate them in
           # standalone mode.
           AC_CONFIG_LINKS(
        hwloc_config_prefix[tests/ports/topology.c]:hwloc_config_prefix[src/topology.c]
	hwloc_config_prefix[tests/ports/traversal.c]:hwloc_config_prefix[src/traversal.c]
	hwloc_config_prefix[tests/ports/topology-synthetic.c]:hwloc_config_prefix[src/topology-synthetic.c]
	hwloc_config_prefix[tests/ports/topology-solaris.c]:hwloc_config_prefix[src/topology-solaris.c]
	hwloc_config_prefix[tests/ports/topology-aix.c]:hwloc_config_prefix[src/topology-aix.c]
	hwloc_config_prefix[tests/ports/topology-osf.c]:hwloc_config_prefix[src/topology-osf.c]
	hwloc_config_prefix[tests/ports/topology-windows.c]:hwloc_config_prefix[src/topology-windows.c]
	hwloc_config_prefix[tests/ports/topology-darwin.c]:hwloc_config_prefix[src/topology-darwin.c]
	hwloc_config_prefix[tests/ports/topology-freebsd.c]:hwloc_config_prefix[src/topology-freebsd.c]
	hwloc_config_prefix[tests/ports/topology-hpux.c]:hwloc_config_prefix[src/topology-hpux.c])
    ])

    # Always generate these files
    AC_CONFIG_FILES(
        hwloc_config_prefix[Makefile]
        hwloc_config_prefix[doc/Makefile]
        hwloc_config_prefix[include/Makefile]
        hwloc_config_prefix[src/Makefile ]
        hwloc_config_prefix[tests/Makefile ]
        hwloc_config_prefix[tests/linux/Makefile]
        hwloc_config_prefix[tests/xml/Makefile]
        hwloc_config_prefix[tests/ports/Makefile]
        hwloc_config_prefix[utils/Makefile]
    )

    # Cleanup
    unset hwloc_config_happy
    AC_LANG_POP

    # Success
    $2
])dnl

#-----------------------------------------------------------------------

# Build HWLOC as a standalone package
AC_DEFUN([HWLOC_STANDALONE],[
    AC_REQUIRE([_HWLOC_DEFINE_ARGS])
    hwloc_mode=standalone
])dnl

#-----------------------------------------------------------------------

# Build HWLOC as an embedded package
AC_DEFUN([HWLOC_EMBEDDED],[
    AC_REQUIRE([_HWLOC_DEFINE_ARGS])
    hwloc_mode=embedded
])dnl

#-----------------------------------------------------------------------

# Specify the symbol prefix
AC_DEFUN([HWLOC_SET_SYMBOL_PREFIX],[
    AC_REQUIRE([_HWLOC_DEFINE_ARGS])
    hwloc_symbol_prefix_value=$1
])dnl

#-----------------------------------------------------------------------

# Internals
AC_DEFUN([_HWLOC_DEFINE_ARGS],[
    # Embedded mode, or standalone?
    AC_ARG_ENABLE([embedded-mode],
                    AC_HELP_STRING([--enable-embedded-mode],
                                   [Using --enable-embedded-mode puts the HWLOC into "embedded" mode.  The default is --disable-embedded-mode, meaning that the HWLOC is in "standalone" mode.]))
    if test "$enable_embedded_mode" = "yes"; then
        hwloc_mode=embedded
    else
        hwloc_mode=standalone
    fi

    # Change the symbol prefix?
    AC_ARG_WITH([hwloc-symbol-prefix],
                AC_HELP_STRING([--with-hwloc-symbol-prefix=STRING],
                               [STRING can be any valid C symbol name.  It will be prefixed to all public HWLOC symbols.  Default: "hwloc_"]))

    # Debug mode?
    # If we're building in embedded mode, default to disabling debug.
    AS_IF([test "$hwloc_mode" = "embedded"], [enable_debug=no])
    AC_ARG_ENABLE([debug],
                  AC_HELP_STRING([--enable-debug],
                                 [Using --enable-debug enables various hwloc maintainer-level debugging controls.  This option is not recomended for end users.]))

    # If we're building in embedded mode, default to disabling the docs.
    AS_IF([test "$hwloc_mode" = "embedded"], [enable_doxygen=no])
    AC_ARG_ENABLE([doxygen],
        [AC_HELP_STRING([--enable-doxygen],
                        [enable support for building Doxygen documentation (note that this option is ONLY relevant in developer builds; Doxygen documentation is pre-built for tarball builds and this option is therefore ignored)])])

    AC_ARG_ENABLE(picky,
                  AC_HELP_STRING([--disable-picky],
                                 [When in developer checkouts of hwloc and compiling with gcc, the default is to enable maximum compiler pickyness.  Using --disable-picky or --enable-picky overrides any default setting]))

    # If we're building in embedded mode, default to disabling Cairo.
    AS_IF([test "$hwloc_mode" = "embedded"], [enable_cairo=no])
    AC_ARG_ENABLE([cairo],
                  AS_HELP_STRING([--disable-cairo], 
                                 [Disable the Cairo back-end of hwloc's lstopo command]))

    # If we're building in embedded mode, default to disable XML.
    AS_IF([test "$hwloc_mode" = "embedded"], [enable_xml=no])
    AC_ARG_ENABLE([xml],
                  AS_HELP_STRING([--disable-xml], 
		                 [Disable the XML back-end of hwloc's lstopo command]))
])dnl

#-----------------------------------------------------------------------

# This must be a standalone routine so that it can be called both by
# HWLOC_INIT and an external caller (if HWLOC_INIT is not invoked).
AC_DEFUN([HWLOC_DO_AM_CONDITIONALS],[
    AS_IF([test "$hwloc_did_am_conditionals" != "yes"],[
        AM_CONDITIONAL([HWLOC_BUILD_STANDALONE], [test "$hwloc_mode" = "standalone"])

        AM_CONDITIONAL([HWLOC_HAVE_GCC], [test "x$GCC" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_MS_LIB], [test "x$HWLOC_MS_LIB" != "x"])
        AM_CONDITIONAL([HWLOC_HAVE_OPENAT], [test "x$have_openat" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_LINUX_LIBNUMA],
                       [test "x$have_linux_libnuma" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_SCHED_SETAFFINITY],
                       [test "x$have_sched_setaffinity" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_LIBIBVERBS], 
                       [test "x$hwloc_have_libibverbs" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_CAIRO], [test "x$enable_cairo" != "xno"])
        AM_CONDITIONAL([HWLOC_HAVE_XML], [test "x$enable_xml" != "xno"])

        AM_CONDITIONAL([HWLOC_BUILD_DOXYGEN],
                       [test "x$hwloc_generate_doxs" = "xyes"])
        AM_CONDITIONAL([HWLOC_BUILD_README], 
                       [test "x$hwloc_generate_readme" = "xyes" -a \( "x$hwloc_install_doxs" = "xyes" -o "x$hwloc_generate_doxs" = "xyes" \) ])
        AM_CONDITIONAL([HWLOC_INSTALL_DOXYGEN], 
                       [test "x$hwloc_install_doxs" = "xyes"])

        AM_CONDITIONAL([HWLOC_HAVE_LINUX], [test "x$hwloc_linux" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_IRIX], [test "x$hwloc_irix" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_DARWIN], [test "x$hwloc_darwin" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_FREEBSD], [test "x$hwloc_freebsd" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_SOLARIS], [test "x$hwloc_solaris" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_AIX], [test "x$hwloc_aix" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_OSF], [test "x$hwloc_osf" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_HPUX], [test "x$hwloc_hpux" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_WINDOWS], [test "x$hwloc_windows" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_MINGW32], 
                       [test "x$hwloc_target_os" = "xmingw32"])

        AM_CONDITIONAL([HWLOC_HAVE_X86_32], [test "x$hwloc_x86_32" = "xyes"])
        AM_CONDITIONAL([HWLOC_HAVE_X86_64], [test "x$hwloc_x86_64" = "xyes"])
        AM_CONDITIONAL([HWLOC_DOXYGEN_BROKEN_SHORT_NAMES], [test "$DOXYGEN_VERSION" = "1.6.2"])
        AM_CONDITIONAL([HWLOC_HAVE_CPUID], [test "x$hwloc_have_cpuid" = "xyes"])
    ])
    hwloc_did_am_conditionals=yes
])dnl

#-----------------------------------------------------------------------

dnl We only build documentation if this is a developer checkout.
dnl Distribution tarballs just install pre-built docuemntation that was
dnl included in the tarball.

AC_DEFUN([_HWLOC_SETUP_DOCS],[
    AC_MSG_CHECKING([if this is a developer build])
    AS_IF([test "$hwloc_mode" = "embedded" -o ! -d "$srcdir/.svn" -a ! -d "$srcdir/.hg" -a ! -d "$srcdir/.git"],
          [AC_MSG_RESULT([no (doxygen generation is optional)])],
          [AC_MSG_RESULT([yes])])
    
    # Generating the doxygen output requires a few tools.  If we
    # don't have all of them, refuse the build the docs.
    AC_ARG_VAR([DOXYGEN], [Location of the doxygen program (required for building the hwloc doxygen documentation)])
    AC_PATH_TOOL([DOXYGEN], [doxygen])
    DOXYGEN_VERSION=`doxygen --version 2> /dev/null`
    
    AC_ARG_VAR([PDFLATEX], [Location of the pdflatex program (required for building the hwloc doxygen documentation)])
    AC_PATH_TOOL([PDFLATEX], [pdflatex])
    
    AC_ARG_VAR([MAKEINDEX], [Location of the makeindex program (required for building the hwloc doxygen documentation)])
    AC_PATH_TOOL([MAKEINDEX], [makeindex])
    
    AC_ARG_VAR([FIG2DEV], [Location of the fig2dev program (required for building the hwloc doxygen documentation)])
    AC_PATH_TOOL([FIG2DEV], [fig2dev])
    
    AC_MSG_CHECKING([if can build doxygen docs])
    AS_IF([test "x$DOXYGEN" != "x" -a "x$PDFLATEX" != "x" -a "x$MAKEINDEX" != "x" -a "x$FIG2DEV" != "x"],
                 [hwloc_generate_doxs=yes], [hwloc_generate_doxs=no])
    AC_MSG_RESULT([$hwloc_generate_doxs])
    
    # Making the top-level README requires w3m or lynx.
    AC_ARG_VAR([W3M], [Location of the w3m program (required to building the top-level hwloc README file)])
    AC_PATH_TOOL([W3M], [w3m])
    AC_ARG_VAR([LYNX], [Location of the lynx program (required to building the top-level hwloc README file)])
    AC_PATH_TOOL([LYNX], [lynx])
    
    AC_MSG_CHECKING([if can build top-level README])
    AS_IF([test "x$W3M" != "x"],
          [hwloc_generate_readme=yes
           HWLOC_W3_GENERATOR=$W3M],
          [AS_IF([test "x$LYNX" != "x"],
                 [hwloc_generate_readme=yes
                  HWLOC_W3_GENERATOR="$LYNX -dump -nolist"],
                 [hwloc_generate_readme=no])])
    AC_SUBST(HWLOC_W3_GENERATOR)
    AC_MSG_RESULT([$hwloc_generate_readme])
    
    # If any one of the above tools is missing, we will refuse to make dist.
    
    AC_MSG_CHECKING([if will build doxygen docs])
    AS_IF([test "x$hwloc_generate_doxs" = "xyes" -a "x$enable_doxygen" != "xno"],
          [], [hwloc_generate_doxs=no])
    AC_MSG_RESULT([$hwloc_generate_doxs])
    
    # See if we want to install the doxygen docs
    AC_MSG_CHECKING([if will install doxygen docs])
    AS_IF([test "x$hwloc_generate_doxs" = "xyes" -o \
    	    -f "$srcdir/doc/doxygen-doc/man/man3/hwloc_distribute.3" -a \
    	    -f "$srcdir/doc/doxygen-doc/hwloc-a4.pdf" -a \
    	    -f "$srcdir/doc/doxygen-doc/hwloc-letter.pdf"],
          [hwloc_install_doxs=yes],
          [hwloc_install_doxs=no])
    AC_MSG_RESULT([$hwloc_install_doxs])
    
    # For the common developer case, if we're in a developer checkout and
    # using the GNU compilers, turn on maximum warnings unless
    # specifically disabled by the user.
    
    AC_MSG_CHECKING([whether to enable "picky" compiler mode])
    want_picky=0
    AS_IF([test "$GCC" = "yes"],
          [AS_IF([test -d "$srcdir/.svn" -o -d "$srcdir/.hg"],
                 [want_picky=1])])
    if test "$enable_picky" = "yes"; then
        if test "$GCC" = "yes"; then
            AC_MSG_RESULT([yes])
            want_picky=1
        else
            AC_MSG_RESULT([no])
            AC_MSG_WARN([Warning: --enable-picky used, but is currently only defined for the GCC compiler set -- automatically disabled])
            want_picky=0
        fi
    elif test "$enable_picky" = "no"; then
        AC_MSG_RESULT([no])
        want_picky=0
    else
        if test "$want_picky" = 1; then
            AC_MSG_RESULT([yes (default)])
        else
            AC_MSG_RESULT([no (default)])
        fi
    fi
    if test "$want_picky" = 1; then
        add="-Wall -Wunused-parameter -Wundef -Wno-long-long -Wsign-compare"
        add="$add -Wmissing-prototypes -Wstrict-prototypes"
        add="$add -Wcomment -pedantic"

        CFLAGS="$CFLAGS $add"
    fi
])

#-----------------------------------------------------------------------

dnl HWLOC_GCC_FLAGS
dnl
dnl Substitute in `GCC_CFLAGS' GCC-specific flags.
AC_DEFUN([_HWLOC_GCC_FLAGS], [
  # GCC specifics.
  if test "x$GCC" = "xyes"; then
    HWLOC_GCC_CFLAGS="-std=gnu99 -Wall -Wmissing-prototypes -Wundef"
    HWLOC_GCC_CFLAGS="$HWLOC_GCC_CFLAGS -Wpointer-arith -Wcast-align"
  fi
  AC_SUBST(HWLOC_GCC_CFLAGS)
])

#-----------------------------------------------------------------------

AC_DEFUN([_HWLOC_CHECK_DIFF_U], [
  AC_MSG_CHECKING([whether diff accepts -u])
  if diff -u /dev/null /dev/null 2> /dev/null
  then
    HWLOC_DIFF_U="-u"
  else
    HWLOC_DIFF_U=""
  fi
  AC_SUBST([HWLOC_DIFF_U])
  AC_MSG_RESULT([$HWLOC_DIFF_U])
])

#-----------------------------------------------------------------------

dnl HWLOC_CHECK_DECL
dnl
dnl Check declaration of given function by trying to call it with an insane
dnl number of arguments (10). Success means the compiler couldn't really check.
AC_DEFUN([_HWLOC_CHECK_DECL], [
  AC_MSG_CHECKING([whether function $1 is declared])
  AC_REQUIRE([AC_PROG_CC])
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT([$4])],[$1(1,2,3,4,5,6,7,8,9,10);])],
    ac_res=no
    $3,
    ac_res=yes
    $2
  )
  AC_MSG_RESULT([$ac_res])
])

#-----------------------------------------------------------------------

dnl HWLOC_CHECK_DECLS
dnl
dnl Same as HWLOCK_CHECK_DECL, but defines HAVE_DECL_foo to 1 or 0 depending on
dnl the result.
AC_DEFUN([_HWLOC_CHECK_DECLS], [
  HWLOC_CHECK_DECL([$1], [ac_have_decl=1], [ac_have_decl=0], [$4])
  AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_DECL_$1]), [$ac_have_decl],
    [Define to 1 if you have the declaration of `$1', and to 0 if you don't])
])

