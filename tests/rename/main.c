#define _GNU_SOURCE 1
#include "sched.h"

/* all headers should be included */
#include "hwloc.h"
#include "hwloc/bitmap.h"
#include "hwloc/helper.h"
#include "hwloc/plugins.h"

/* enable those that have the right dependencies on your machine */
//#include "hwloc/opencl.h"
//#include "hwloc/cuda.h"
//#include "hwloc/cudart.h"
//#include "hwloc/nvml.h"
#include "hwloc/glibc-sched.h"
#include "hwloc/linux.h"
#include "hwloc/linux-libnuma.h"
//#include "hwloc/myriexpress.h"
//#include "hwloc/openfabrics-verbs.h"

#include "private/autogen/config.h"
#include "private/components.h"
#include "private/cpuid.h"
#include "private/debug.h"
#include "private/misc.h"
#include "private/private.h"
#include "private/solaris-chiptype.h"
#include "private/xml.h"
