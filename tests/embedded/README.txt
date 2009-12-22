This test is meant to be run manually; it is not part of "make check".

Someday I may figure out how to make this part of "make check", but
today is not that day.  :-)

You need to expand a distribution hwloc tarball in this directory and
rename the top-level directory from hwloc-<version>/ to hwloc-tree/.
Then run ./autogen.sh, ./configure, and make.  

The test is that:

 - autogen.sh runs properly and to completion
 - configure runs properly and to completion
 - make runs properly and to completion
 - you can run the resulting "./main" executable and it properly shows
   the hwloc depth of the current machine

If you look at configure.ac, you see that it uses the HWLOC m4 macros
to build the hwloc located at hwloc-tree/, and renames all the symbols
from "hwloc_<foo>" to "mytest_<foo>".  The main.c source calls several
hwloc functions via the "mytest_<foo>" symbols.  

Bottom line: if the "main" executable runs and prints the current
depth, the embedding should be working properly.
