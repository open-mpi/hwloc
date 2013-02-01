all-local: prepare _missing _useless restore

prepare:
	@echo "# Enabling renaming in headers"
	@sed	-e '/^#define HWLOC_SYM_TRANSFORM 0/s/0/1/'			\
		-e '/^#define HWLOC_SYM_PREFIX hwloc_/s/hwloc_/foobar_/1'	\
		-e '/^#define HWLOC_SYM_PREFIX_CAPS HWLOC_/s/HWLOC_/FOOBAR_/2'	\
		-i $(abs_top_builddir)/include/hwloc/autogen/config.h $(abs_top_builddir)/include/private/autogen/config.h
	@echo "# done"

restore:
	@echo "# Restoring original headers"
	@sed	-e '/^#define HWLOC_SYM_TRANSFORM 1/s/1/0/'			\
		-e '/^#define HWLOC_SYM_PREFIX foobar_/s/foobar_/hwloc_/1'	\
		-e '/^#define HWLOC_SYM_PREFIX_CAPS FOOBAR_/s/FOOBAR_/HWLOC_/2'	\
		-i $(abs_top_builddir)/include/hwloc/autogen/config.h $(abs_top_builddir)/include/private/autogen/config.h
	@echo "# done"

_useless:
	@echo "# Finding useless renaming"
	@grep '^#define' $(abs_top_srcdir)/include/hwloc/rename.h \
		| awk '{print $$2}' \
		| egrep -v '(HWLOC_RENAME_H|HWLOC_MUNGE_NAME|HWLOC_NAME)' \
		| while read name ; do \
			grep $$name -rwH $(abs_top_srcdir)/include/ \
			| grep -v rename.h: | grep -v .svn >/dev/null \
			|| echo $$name; \
			done
	@echo "# done"

useless: prepare _useless restore

_missing: prepare
	@echo "# Finding missing renaming"
	@cpp $(CPPFLAGS) $(srcdir)/main.c -I $(abs_top_srcdir)/include -I $(abs_top_builddir)/include \
		| egrep -i '(^| |\*)hwloc_'
	@echo "# done"

missing: prepare _missing restore

EXTRA_DIST	=	main.c
