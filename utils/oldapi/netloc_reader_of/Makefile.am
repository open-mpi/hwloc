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
        netloc_reader_of

netloc_reader_of_SOURCES = \
        perl_json_support.h \
        netloc_reader_of.c

netloc_reader_of_LDADD = \
        $(top_builddir)/netloc/libnetloc.la

#
# Below adapted from:
# http://www.gnu.org/software/automake/manual/html_node/Scripts.html
#
bin_SCRIPTS = \
        Perl_OF_support.pm \
        netloc_reader_of_floodlight \
        netloc_reader_of_opendaylight
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = \
        Perl_OF_support.pm.in \
        netloc_reader_of_floodlight.pl \
        netloc_reader_of_opendaylight.pl

do_subst = \
    sed -e 's,[@]datadir[@],$(datadir),g' \
        -e 's,[@]bindir[@],$(bindir),g' \
        -e 's,[@]libdir[@],$(libdir),g'

Perl_OF_support.pm: Perl_OF_support.pm.in
	$(do_subst) < $(srcdir)/Perl_OF_support.pm.in > Perl_OF_support.pm

netloc_reader_of_floodlight: netloc_reader_of_floodlight.pl
	$(do_subst) < $(srcdir)/netloc_reader_of_floodlight.pl > netloc_reader_of_floodlight
	chmod +x netloc_reader_of_floodlight

netloc_reader_of_opendaylight: netloc_reader_of_opendaylight.pl
	$(do_subst) < $(srcdir)/netloc_reader_of_opendaylight.pl > netloc_reader_of_opendaylight
	chmod +x netloc_reader_of_opendaylight
