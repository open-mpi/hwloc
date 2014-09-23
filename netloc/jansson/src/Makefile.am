EXTRA_DIST = jansson.def

# NETLOC: do not install these headers
headers = jansson.h jansson_config.h

# NETLOC: Turn this into a non-installable convenience LT library
noinst_LTLIBRARIES = libjansson.la
libjansson_la_SOURCES = \
        $(headers) \
	dump.c \
	error.c \
	hashtable.c \
	hashtable.h \
	hashtable_seed.c \
	jansson_private.h \
	load.c \
	lookup3.h \
	memory.c \
	pack_unpack.c \
	strbuffer.c \
	strbuffer.h \
	strconv.c \
	utf.c \
	utf.h \
	value.c
# NETLOC: Remove -version-info from LDFLAGS; it's ignored (and warned
# about) for LT convenience libs
libjansson_la_LDFLAGS = \
	-no-undefined \
	-export-symbols-regex '^json_'
