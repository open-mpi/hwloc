# NETLOC: remove win32 and cmake from EXTRA_DIST
#EXTRA_DIST = CHANGES LICENSE README.rst win32 CMakeLists.txt cmake
EXTRA_DIST = CHANGES LICENSE README.rst CMakeLists.txt
# NETLOC: We don't care about building the doc or test subdirs
#SUBDIRS = doc src test
SUBDIRS = src

# "make distcheck" builds the dvi target, so use it to check that the
# documentation is built correctly.
# NETLOC: change target to dvi-local (not dvi), per Automake warning
dvi-local:
	$(MAKE) SPHINXOPTS_EXTRA=-W html

# NETLOC: we don't need jansson.pc
#pkgconfigdir = $(libdir)/pkgconfig
#pkgconfig_DATA = jansson.pc

if GCC
# These flags are gcc specific
export AM_CFLAGS = -Wall -Wextra -Wdeclaration-after-statement
endif
