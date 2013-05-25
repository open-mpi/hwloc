all-local: needed useless

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

dirs:
	mkdir -p include/hwloc/autogen include/private/autogen

include/hwloc/autogen/config.h: $(HWLOC_top_builddir)/include/hwloc/autogen/config.h
	sed	-e '/^#define HWLOC_SYM_TRANSFORM 0/s/0/1/'			\
		-e '/^#define HWLOC_SYM_PREFIX hwloc_/s/hwloc_/foobar_/1'	\
		-e '/^#define HWLOC_SYM_PREFIX_CAPS HWLOC_/s/HWLOC_/FOOBAR_/2'	\
		$< > $@

include/private/autogen/config.h: $(HWLOC_top_builddir)/include/private/autogen/config.h
	sed	-e '/^#define HWLOC_SYM_TRANSFORM 0/s/0/1/'			\
		-e '/^#define HWLOC_SYM_PREFIX hwloc_/s/hwloc_/foobar_/1'	\
		-e '/^#define HWLOC_SYM_PREFIX_CAPS HWLOC_/s/HWLOC_/FOOBAR_/2'	\
		$< > $@

useless:
	@echo "# Finding useless renaming"
	@if grep '^#define' $(HWLOC_top_srcdir)/include/hwloc/rename.h \
		| awk '{print $$2}' \
		| egrep -v '(HWLOC_RENAME_H|HWLOC_MUNGE_NAME|HWLOC_NAME)' \
		| while read name ; do \
			grep $$name -rwH $(HWLOC_top_srcdir)/include/ \
			| grep -v rename.h: | grep -v .svn >/dev/null \
			|| echo $$name; \
			done \
	 | grep '' ; then false ; else true ; fi
	@echo "# done"

# hwloc_uint64_t is ignored because it's always a typedef
needed: dirs include/hwloc/autogen/config.h include/private/autogen/config.h
	@echo "# Finding needed renaming"
	@echo "# using $(TEST_CPPFLAGS)"
	@if cpp $(CPPFLAGS) $(TEST_CPPFLAGS) $(srcdir)/main.c -I include -I $(HWLOC_top_srcdir)/include -I $(HWLOC_top_builddir)/include \
		| grep -vw hwloc_uint64_t \
		| egrep -i '(^| |\*)hwloc_' ; then false ; else true ; fi
	@echo "# done"

clean-local:
	rm -rf include

EXTRA_DIST	=	main.c
