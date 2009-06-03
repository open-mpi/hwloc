# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am
include $(top_srcdir)/doxample.am

if DX_COND_man

dist_man1_MANS = @DX_DOCDIR@/man/man1/libtopology.h.1

$(dist_man1_MANS): doxygen-doc

endif

MOSTLYCLEANFILES = $(DX_CLEANFILES)

EXTRA_DITS = $(DX_CONFIG)

#info_TEXINFOS = libtopology.texi
