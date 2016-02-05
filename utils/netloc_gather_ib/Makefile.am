# Copyright © 2013 Cisco Systems, Inc.  All rights reserved.
# Copyright © 2013      University of Wisconsin-La Crosse.
#                         All rights reserved.
#
# Copyright © 2016 Inria.  All rights reserved.
# See COPYING in top-level directory.
#
# $HEADER$
#

#
# Below adapted from:
# http://www.gnu.org/software/automake/manual/html_node/Scripts.html
#
bin_SCRIPTS = \
        netloc_ib_extract_dats \
        netloc_ib_gather_raw
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST = \
        netloc_ib_extract_dats.pl

do_subst = \
    sed -e 's,[@]datadir[@],$(datadir),g' \
        -e 's,[@]bindir[@],$(bindir),g' \
        -e 's,[@]libdir[@],$(libdir),g'

netloc_ib_extract_dats: netloc_ib_extract_dats.pl
	$(do_subst) < $(srcdir)/netloc_ib_extract_dats.pl > netloc_ib_extract_dats
	chmod +x netloc_ib_extract_dats

install-exec-hook:
	$(SED) -e 's/HWLOC_top_builddir\/utils\/lstopo/bindir/' -e '/HWLOC_top_builddir/d' $(DESTDIR)$(bindir)/netloc_ib_gather_raw > $(DESTDIR)$(bindir)/netloc_ib_gather_raw.tmp && mv -f $(DESTDIR)$(bindir)/netloc_ib_gather_raw.tmp $(DESTDIR)$(bindir)/netloc_ib_gather_raw
	chmod +x $(DESTDIR)$(bindir)/netloc_ib_gather_raw
