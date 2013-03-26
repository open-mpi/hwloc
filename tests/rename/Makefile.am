all-local: prepare _needed _useless restore

TEST_CPPFLAGS =
if HWLOC_HAVE_LINUX
TEST_CPPFLAGS += -DHWLOC_TEST_RENAME_LINUX=1
endif HWLOC_HAVE_LINUX
if HWLOC_HAVE_LINUX_LIBNUMA
TEST_CPPFLAGS += -DHWLOC_TEST_RENAME_LINUX_LIBNUMA=1
endif HWLOC_HAVE_LINUX_LIBNUMA
if HWLOC_HAVE_SCHED_SETAFFINITY
TEST_CPPFLAGS += -DHWLOC_TEST_RENAME_GLIBC_SCHED=1
endif HWLOC_HAVE_SCHED_SETAFFINITY
if HWLOC_HAVE_LIBIBVERBS
TEST_CPPFLAGS += -DHWLOC_TEST_RENAME_OPENFABRICS_VERBS=1
endif HWLOC_HAVE_LIBIBVERBS
if HWLOC_HAVE_MYRIEXPRESS
TEST_CPPFLAGS += -DHWLOC_TEST_RENAME_MYRIEXPRESS=1
endif HWLOC_HAVE_MYRIEXPRESS
if HWLOC_HAVE_OPENCL
TEST_CPPFLAGS += -DHWLOC_TEST_RENAME_OPENCL=1
endif HWLOC_HAVE_OPENCL
if HWLOC_HAVE_CUDA
TEST_CPPFLAGS += -DHWLOC_TEST_RENAME_CUDA=1
endif HWLOC_HAVE_CUDA
if HWLOC_HAVE_CUDART
TEST_CPPFLAGS += -DHWLOC_TEST_RENAME_CUDART=1
endif HWLOC_HAVE_CUDART
if HWLOC_HAVE_NVML
TEST_CPPFLAGS += -DHWLOC_TEST_RENAME_NVML=1
endif HWLOC_HAVE_NVML

prepare:
	@echo "# Enabling renaming in headers"
	@sed	-e '/^#define HWLOC_SYM_TRANSFORM 0/s/0/1/'			\
		-e '/^#define HWLOC_SYM_PREFIX hwloc_/s/hwloc_/foobar_/1'	\
		-e '/^#define HWLOC_SYM_PREFIX_CAPS HWLOC_/s/HWLOC_/FOOBAR_/2'	\
		-i $(HWLOC_top_builddir)/include/hwloc/autogen/config.h $(HWLOC_top_builddir)/include/private/autogen/config.h
	@echo "# done"

restore:
	@echo "# Restoring original headers"
	@sed	-e '/^#define HWLOC_SYM_TRANSFORM 1/s/1/0/'			\
		-e '/^#define HWLOC_SYM_PREFIX foobar_/s/foobar_/hwloc_/1'	\
		-e '/^#define HWLOC_SYM_PREFIX_CAPS FOOBAR_/s/FOOBAR_/HWLOC_/2'	\
		-i $(HWLOC_top_builddir)/include/hwloc/autogen/config.h $(HWLOC_top_builddir)/include/private/autogen/config.h
	@echo "# done"

_useless:
	@echo "# Finding useless renaming"
	@grep '^#define' $(HWLOC_top_srcdir)/include/hwloc/rename.h \
		| awk '{print $$2}' \
		| egrep -v '(HWLOC_RENAME_H|HWLOC_MUNGE_NAME|HWLOC_NAME)' \
		| while read name ; do \
			grep $$name -rwH $(HWLOC_top_srcdir)/include/ \
			| grep -v rename.h: | grep -v .svn >/dev/null \
			|| echo $$name; \
			done
	@echo "# done"

useless: prepare _useless restore

_needed:
	@echo "# Finding needed renaming"
	@echo "# using $(TEST_CPPFLAGS)"
	@cpp $(CPPFLAGS) $(TEST_CPPFLAGS) $(srcdir)/main.c -I $(HWLOC_top_srcdir)/include -I $(HWLOC_top_builddir)/include \
		| egrep -i '(^| |\*)hwloc_'
	@echo "# done"

needed: prepare _needed restore

EXTRA_DIST	=	main.c
