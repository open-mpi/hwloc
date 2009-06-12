# Copyright 2009 INRIA, Université Bordeaux 1

include $(top_srcdir)/libtopology.am
include $(top_srcdir)/doxample.am

MOSTLYCLEANFILES =
CLEANFILES =
EXTRA_DIST = $(DX_CONFIG) \
	topology.doxy \
	images/dudley.png \
	images/emmett.png \
	images/hagrid.png

#
# Documentation is already available in release tarballs.
#

if TOPO_INSTALL_MAN
dist_man3_MANS = \
	@DX_DOCDIR@/man/man3/TOPO_CPUSET_CPU.3 \
	@DX_DOCDIR@/man/man3/TOPO_CPUSET_FULL.3 \
	@DX_DOCDIR@/man/man3/TOPO_CPUSET_STRING_LENGTH.3 \
	@DX_DOCDIR@/man/man3/TOPO_CPUSET_ZERO.3 \
	@DX_DOCDIR@/man/man3/TOPO_FLAGS_WHOLE_SYSTEM.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_CACHE.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_CORE.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_MISC.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_MACHINE.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_NODE.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_PROC.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_SOCKET.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_SYSTEM.3 \
	@DX_DOCDIR@/man/man3/TOPO_OBJ_TYPE_MAX.3 \
	@DX_DOCDIR@/man/man3/TOPO_PRIxCPUSET.3 \
	@DX_DOCDIR@/man/man3/TOPO_TYPE_DEPTH_MULTIPLE.3 \
	@DX_DOCDIR@/man/man3/TOPO_TYPE_DEPTH_UNKNOWN.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_all_but_cpu.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_andset.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_clearset.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_clr.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_compar.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_compar_first.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_cpu.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_fill.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_first.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_foreach_begin.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_foreach_end.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_from_ith_ulong.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_from_string.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_from_ulong.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_intersects.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_isequal.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_isfull.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_isincluded.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_isset.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_iszero.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_orset.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_set.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_set_range.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_snprintf.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_t.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_to_ith_ulong.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_to_ulong.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_weight.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_xorset.3 \
	@DX_DOCDIR@/man/man3/topo_cpuset_zero.3 \
	@DX_DOCDIR@/man/man3/topo_flags_e.3 \
	@DX_DOCDIR@/man/man3/topo_get_closest_objs.3 \
	@DX_DOCDIR@/man/man3/topo_get_common_ancestor_obj.3 \
	@DX_DOCDIR@/man/man3/topo_get_cpuset_covering_cache.3 \
	@DX_DOCDIR@/man/man3/topo_get_cpuset_covering_child.3 \
	@DX_DOCDIR@/man/man3/topo_get_cpuset_covering_obj.3 \
	@DX_DOCDIR@/man/man3/topo_get_cpuset_objs.3 \
	@DX_DOCDIR@/man/man3/topo_get_depth_nbobjs.3 \
	@DX_DOCDIR@/man/man3/topo_get_depth_type.3 \
	@DX_DOCDIR@/man/man3/topo_get_next_child.3 \
	@DX_DOCDIR@/man/man3/topo_get_next_obj.3 \
	@DX_DOCDIR@/man/man3/topo_get_next_obj_by_depth.3 \
	@DX_DOCDIR@/man/man3/topo_get_next_obj_above_cpuset.3 \
	@DX_DOCDIR@/man/man3/topo_get_next_obj_above_cpuset_by_depth.3 \
	@DX_DOCDIR@/man/man3/topo_get_next_obj_below_cpuset_by_depth.3 \
	@DX_DOCDIR@/man/man3/topo_get_next_obj_below_cpuset.3 \
	@DX_DOCDIR@/man/man3/topo_get_obj.3 \
	@DX_DOCDIR@/man/man3/topo_get_obj_below_cpuset_by_depth.3 \
	@DX_DOCDIR@/man/man3/topo_get_obj_order_type.3 \
	@DX_DOCDIR@/man/man3/topo_get_obj_type_order.3 \
	@DX_DOCDIR@/man/man3/topo_get_shared_cache_above.3 \
	@DX_DOCDIR@/man/man3/topo_get_system_obj.3 \
	@DX_DOCDIR@/man/man3/topo_get_type_depth.3 \
	@DX_DOCDIR@/man/man3/topo_get_type_or_above_depth.3 \
	@DX_DOCDIR@/man/man3/topo_get_type_or_below_depth.3 \
	@DX_DOCDIR@/man/man3/topo_obj.3 \
	@DX_DOCDIR@/man/man3/topo_obj_cpuset_snprintf.3 \
	@DX_DOCDIR@/man/man3/topo_obj_is_in_subtree.3 \
	@DX_DOCDIR@/man/man3/topo_obj_snprintf.3 \
	@DX_DOCDIR@/man/man3/topo_obj_t.3 \
	@DX_DOCDIR@/man/man3/topo_obj_topo_obj_attr_u.3 \
	@DX_DOCDIR@/man/man3/topo_obj_topo_obj_attr_u_topo_cache_attr_u.3 \
	@DX_DOCDIR@/man/man3/topo_obj_topo_obj_attr_u_topo_misc_attr_u.3 \
	@DX_DOCDIR@/man/man3/topo_obj_topo_obj_attr_u_topo_memory_attr_u.3 \
	@DX_DOCDIR@/man/man3/topo_obj_type_e.3 \
	@DX_DOCDIR@/man/man3/topo_obj_type_string.3 \
	@DX_DOCDIR@/man/man3/topo_obj_type_of_string.3 \
	@DX_DOCDIR@/man/man3/topo_obj_type_t.3 \
	@DX_DOCDIR@/man/man3/topo_set_cpubind.3 \
	@DX_DOCDIR@/man/man3/topo_topology_destroy.3 \
	@DX_DOCDIR@/man/man3/topo_topology_get_info.3 \
	@DX_DOCDIR@/man/man3/topo_topology_ignore_all_keep_structure.3 \
	@DX_DOCDIR@/man/man3/topo_topology_ignore_type.3 \
	@DX_DOCDIR@/man/man3/topo_topology_ignore_type_keep_structure.3 \
	@DX_DOCDIR@/man/man3/topo_topology_info.3 \
	@DX_DOCDIR@/man/man3/topo_topology_init.3 \
	@DX_DOCDIR@/man/man3/topo_topology_load.3 \
	@DX_DOCDIR@/man/man3/topo_topology_set_flags.3 \
	@DX_DOCDIR@/man/man3/topo_topology_set_fsys_root.3 \
	@DX_DOCDIR@/man/man3/topo_topology_set_synthetic.3 \
	@DX_DOCDIR@/man/man3/topo_topology_set_xml.3 \
	@DX_DOCDIR@/man/man3/topo_topology_t.3 \
	@DX_DOCDIR@/man/man3/topology_binding.3 \
	@DX_DOCDIR@/man/man3/topology_configuration.3 \
	@DX_DOCDIR@/man/man3/topology_conversion.3 \
	@DX_DOCDIR@/man/man3/topology_cpuset.3 \
	@DX_DOCDIR@/man/man3/topology_creation.3 \
	@DX_DOCDIR@/man/man3/topology_info.3 \
	@DX_DOCDIR@/man/man3/topology_information.3 \
	@DX_DOCDIR@/man/man3/topology_objects.3 \
	@DX_DOCDIR@/man/man3/topology_traversal.3

EXTRA_DIST += \
	@DX_DOCDIR@/man/man3/arity.3 \
	@DX_DOCDIR@/man/man3/attr.3 \
	@DX_DOCDIR@/man/man3/cache.3 \
	@DX_DOCDIR@/man/man3/children.3 \
	@DX_DOCDIR@/man/man3/cpuset.3 \
	@DX_DOCDIR@/man/man3/cpuset.h.3 \
	@DX_DOCDIR@/man/man3/depth.3 \
	@DX_DOCDIR@/man/man3/dmi_board_name.3 \
	@DX_DOCDIR@/man/man3/dmi_board_vendor.3 \
	@DX_DOCDIR@/man/man3/father.3 \
	@DX_DOCDIR@/man/man3/first_child.3 \
	@DX_DOCDIR@/man/man3/helper.h.3 \
	@DX_DOCDIR@/man/man3/huge_page_free.3 \
	@DX_DOCDIR@/man/man3/huge_page_size_kB.3 \
	@DX_DOCDIR@/man/man3/sibling_rank.3 \
	@DX_DOCDIR@/man/man3/is_fake.3 \
	@DX_DOCDIR@/man/man3/last_child.3 \
	@DX_DOCDIR@/man/man3/machine.3 \
	@DX_DOCDIR@/man/man3/memory_kB.3 \
	@DX_DOCDIR@/man/man3/misc.3 \
	@DX_DOCDIR@/man/man3/next_cousin.3 \
	@DX_DOCDIR@/man/man3/next_sibling.3 \
	@DX_DOCDIR@/man/man3/node.3 \
	@DX_DOCDIR@/man/man3/logical_index.3 \
	@DX_DOCDIR@/man/man3/os_index.3 \
	@DX_DOCDIR@/man/man3/prev_cousin.3 \
	@DX_DOCDIR@/man/man3/prev_sibling.3 \
	@DX_DOCDIR@/man/man3/s.3 \
	@DX_DOCDIR@/man/man3/system.3 \
	@DX_DOCDIR@/man/man3/topology.doxy.3 \
	@DX_DOCDIR@/man/man3/topology.h.3 \
	@DX_DOCDIR@/man/man3/type.3 \
	@DX_DOCDIR@/man/man3/userdata.3
endif

if TOPO_INSTALL_HTML
EXTRA_DIST += \
	@DX_DOCDIR@/html/annotated.html \
	@DX_DOCDIR@/html/classes.html \
	@DX_DOCDIR@/html/cpuset_8h.html \
	@DX_DOCDIR@/html/cpuset_8h_source.html \
	@DX_DOCDIR@/html/doxygen.css \
	@DX_DOCDIR@/html/doxygen.png \
	@DX_DOCDIR@/html/dudley.png \
	@DX_DOCDIR@/html/emmett.png \
	@DX_DOCDIR@/html/files.html \
	@DX_DOCDIR@/html/functions.html \
	@DX_DOCDIR@/html/functions_vars.html \
	@DX_DOCDIR@/html/globals.html \
	@DX_DOCDIR@/html/globals_defs.html \
	@DX_DOCDIR@/html/globals_enum.html \
	@DX_DOCDIR@/html/globals_eval.html \
	@DX_DOCDIR@/html/globals_func.html \
	@DX_DOCDIR@/html/globals_type.html \
	@DX_DOCDIR@/html/group__topology__binding.html \
	@DX_DOCDIR@/html/group__topology__configuration.html \
	@DX_DOCDIR@/html/group__topology__conversion.html \
	@DX_DOCDIR@/html/group__topology__cpuset.html \
	@DX_DOCDIR@/html/group__topology__creation.html \
	@DX_DOCDIR@/html/group__topology__info.html \
	@DX_DOCDIR@/html/group__topology__information.html \
	@DX_DOCDIR@/html/group__topology__objects.html \
	@DX_DOCDIR@/html/group__topology__traversal.html \
	@DX_DOCDIR@/html/hagrid.png \
	@DX_DOCDIR@/html/index.html \
	@DX_DOCDIR@/html/modules.html \
	@DX_DOCDIR@/html/structtopo__cpuset__t.html \
	@DX_DOCDIR@/html/structtopo__obj.html \
	@DX_DOCDIR@/html/structtopo__topology__info.html \
	@DX_DOCDIR@/html/tab_b.gif \
	@DX_DOCDIR@/html/tab_l.gif \
	@DX_DOCDIR@/html/tab_r.gif \
	@DX_DOCDIR@/html/tabs.css \
	@DX_DOCDIR@/html/topology_8doxy.html \
	@DX_DOCDIR@/html/topology_8h.html \
	@DX_DOCDIR@/html/topology_8h_source.html \
	@DX_DOCDIR@/html/uniontopo__obj_1_1topo__obj__attr__u.html
endif

if TOPO_INSTALL_PDF
EXTRA_DIST += \
	@DX_DOCDIR@/libtopology.pdf
endif

if TOPO_WILLHAVE_TAG
EXTRA_DIST += \
	@DX_DOCDIR@/libtopology.tag
endif

install-data-local:
if TOPO_INSTALL_PDF
	$(mkinstalldirs) $(DESTDIR)$(pdfdir)
	$(INSTALL_DATA) @DX_DOCDIR@/libtopology.pdf $(DESTDIR)$(pdfdir)
endif
if TOPO_INSTALL_HTML
	$(mkinstalldirs) $(DESTDIR)$(htmldir)/topology
	$(INSTALL_DATA) @DX_DOCDIR@/html/* $(DESTDIR)$(htmldir)/topology
endif

uninstall-local:
if TOPO_INSTALL_PDF
	$(RM) $(DESTDIR)$(pdfdir)/libtopology.pdf
endif
if TOPO_INSTALL_HTML
	$(RM) $(DESTDIR)$(htmldir)/topology/*
endif

#
# Make sure that the documentation example works
#

TESTS = topo-hello

check_PROGRAMS = $(TESTS)

LIBS = $(top_builddir)/src/libtopology.la

topo-hello.c: $(srcdir)/topology.doxy
	sed -e '1,/   \\code/d' -e '/   \\endcode/,$$d' < $^ > $@

MOSTLYCLEANFILES += topo-hello.c

#
# Rules to regenerate the documentation
#

if TOPO_HAVE_MAN
# Documentation already available, do not force documentation generation
else
if DX_COND_man
# Documentation not available, tell how to generate it
$(dist_man3_MANS): @DX_DOCDIR@/libtopology.tag
endif
endif

if TOPO_HAVE_PDF
else
if DX_COND_pdf
all-local: @DX_DOCDIR@/libtopology.pdf
@DX_DOCDIR@/libtopology.pdf: @DX_DOCDIR@/libtopology.tag
endif
endif

if TOPO_HAVE_HTML
else
if DX_COND_html
all-local: @DX_DOCDIR@/html
	cp $(srcdir)/doxygen.css @DX_DOCDIR@/html/
@DX_DOCDIR@/html: @DX_DOCDIR@/libtopology.tag
endif
endif

#
# What doxygen depends on
#
pkginclude_HEADERS = \
	$(srcdir)/topology.doxy \
	$(top_srcdir)/include/topology.h \
	$(top_srcdir)/include/topology/helper.h \
	$(top_srcdir)/include/topology/cpuset.h

#
# Only remove the documentation for maintainers
#
MAINTAINERCLEANFILES = $(DX_CLEANFILES)

#
# But remove the LaTeX files for the tarball
#
CLEANFILES += \
	@DX_DOCDIR@/latex/Makefile \
	@DX_DOCDIR@/latex/annotated.tex \
	@DX_DOCDIR@/latex/cpuset_8h.tex \
	@DX_DOCDIR@/latex/doxygen.sty \
	@DX_DOCDIR@/latex/dudley.png \
	@DX_DOCDIR@/latex/emmett.png \
	@DX_DOCDIR@/latex/files.tex \
	@DX_DOCDIR@/latex/group__topology__binding.tex \
	@DX_DOCDIR@/latex/group__topology__configuration.tex \
	@DX_DOCDIR@/latex/group__topology__conversion.tex \
	@DX_DOCDIR@/latex/group__topology__cpuset.tex \
	@DX_DOCDIR@/latex/group__topology__creation.tex \
	@DX_DOCDIR@/latex/group__topology__info.tex \
	@DX_DOCDIR@/latex/group__topology__information.tex \
	@DX_DOCDIR@/latex/group__topology__objects.tex \
	@DX_DOCDIR@/latex/group__topology__traversal.tex \
	@DX_DOCDIR@/latex/hagrid.png \
	@DX_DOCDIR@/latex/index.tex \
	@DX_DOCDIR@/latex/modules.tex \
	@DX_DOCDIR@/latex/refman.tex \
	@DX_DOCDIR@/latex/structtopo__cpuset__t.tex \
	@DX_DOCDIR@/latex/structtopo__obj.tex \
	@DX_DOCDIR@/latex/structtopo__obj_1_1topo__obj__attr__u_1_1topo__cache__attr__u.tex \
	@DX_DOCDIR@/latex/structtopo__obj_1_1topo__obj__attr__u_1_1topo__memory__attr__u.tex \
	@DX_DOCDIR@/latex/structtopo__obj_1_1topo__obj__attr__u_1_1topo__misc__attr__u.tex \
	@DX_DOCDIR@/latex/structtopo__topology__info.tex \
	@DX_DOCDIR@/latex/topology_8doxy.tex \
	@DX_DOCDIR@/latex/topology_8h.tex \
	@DX_DOCDIR@/latex/uniontopo__obj_1_1topo__obj__attr__u.tex

doc: $(top_srcdir)/README
$(top_srcdir)/README: @DX_DOCDIR@/html
	LC_ALL=C $(W3M) @DX_DOCDIR@/html/index.html | sed -n -e '/^Introduction$$/,$$p' > $@

DOCUPLOADHOST=sync.bordeaux.inria.fr
DOCUPLOADOPTS=
doc-upload: @DX_DOCDIR@/html
	rsync -avz --delete --chmod=ug+rwX,o+rX $(DOCUPLOADOPTS) @DX_DOCDIR@/html/ $(DOCUPLOADHOST):/web/runtime/libtopology2/doc/
