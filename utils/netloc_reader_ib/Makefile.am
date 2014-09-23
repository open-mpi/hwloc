# Copyright © 2013-2014 Cisco Systems, Inc.  All rights reserved.
# Copyright © 2013      University of Wisconsin-La Crosse.
#                         All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#

AM_CPPFLAGS = \
        $(JANSSON_CPPFLAGS) \
        -I$(top_builddir)/include \
        -I$(top_srcdir)/include

bin_PROGRAMS = \
        netloc_reader_ib

netloc_reader_ib_SOURCES = \
        perl_json_support.h \
        netloc_reader_ib.c

netloc_reader_ib_LDADD = \
        $(top_builddir)/netloc/libnetloc.la

#
# Below adapted from:
# http://www.gnu.org/software/automake/manual/html_node/Scripts.html
#
bin_SCRIPTS = \
        Perl_IB_support.pm \
        netloc_reader_ib_backend_general \
        netloc_reader_ib_backend_log_prep
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = \
        Perl_IB_support.pm.in \
        netloc_reader_ib_backend_general.pl \
        netloc_reader_ib_backend_log_prep.pl

do_subst = \
    sed -e 's,[@]datadir[@],$(datadir),g' \
        -e 's,[@]bindir[@],$(bindir),g' \
        -e 's,[@]libdir[@],$(libdir),g'

Perl_IB_support.pm: Perl_IB_support.pm.in
	$(do_subst) < $(srcdir)/Perl_IB_support.pm.in > Perl_IB_support.pm

netloc_reader_ib_backend_general: netloc_reader_ib_backend_general.pl
	$(do_subst) < $(srcdir)/netloc_reader_ib_backend_general.pl > netloc_reader_ib_backend_general
	chmod +x netloc_reader_ib_backend_general

netloc_reader_ib_backend_log_prep: netloc_reader_ib_backend_log_prep.pl
	$(do_subst) < $(srcdir)/netloc_reader_ib_backend_log_prep.pl > netloc_reader_ib_backend_log_prep
	chmod +x netloc_reader_ib_backend_log_prep
