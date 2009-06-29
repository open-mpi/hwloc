/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 *
 * This software is a computer program whose purpose is to provide
 * abstracted information about the hardware topology.
 *
 * This software is governed by the CeCILL-B license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-B
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL-B license and that you accept its terms.
 */

/* Internals for cpuset API.  */

#ifndef TOPOLOGY_CPUSET_BITS_H
#define TOPOLOGY_CPUSET_BITS_H

#include <topology/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95))
# define __topo_restrict __restrict
#else
# if defined restrict || __STDC_VERSION__ >= 199901L
#  define __topo_restrict restrict
# else
#  define __topo_restrict
# endif
#endif


/**
 * Cpuset internals.
 */

/* size and count of subsets within a set */
#define TOPO_CPUSUBSET_SIZE	TOPO_BITS_PER_LONG
#define TOPO_CPUSUBSET_COUNT	((TOPO_NBMAXCPUS+TOPO_CPUSUBSET_SIZE-1)/TOPO_CPUSUBSET_SIZE)

/* extract a subset from a set using an index or a cpu */
#define TOPO_CPUSUBSET_SUBSET(set,x)		((set).s[x])
#define TOPO_CPUSUBSET_INDEX(cpu)		((cpu)/(TOPO_CPUSUBSET_SIZE))
#define TOPO_CPUSUBSET_CPUSUBSET(set,cpu)	TOPO_CPUSUBSET_SUBSET(set,TOPO_CPUSUBSET_INDEX(cpu))

/* predefined subset values */
#define TOPO_CPUSUBSET_VAL(cpu)		(1UL<<((cpu)%(TOPO_CPUSUBSET_SIZE)))
#define TOPO_CPUSUBSET_ZERO		0UL
#define TOPO_CPUSUBSET_FULL		~0UL

/* Strings always use 32bit groups */
#define TOPO_PRIxCPUSUBSET		"%08lx"
#define TOPO_CPUSET_SUBSTRING_SIZE	32
#define TOPO_CPUSET_SUBSTRING_COUNT	((TOPO_NBMAXCPUS+TOPO_CPUSET_SUBSTRING_SIZE-1)/TOPO_CPUSET_SUBSTRING_SIZE)
#define TOPO_CPUSET_SUBSTRING_LENGTH	(TOPO_CPUSET_SUBSTRING_SIZE/4)



/**
 * ffs helpers.
 */

#ifdef __GNUC__

#  if (__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__) >= 4)
     /* Starting from 3.4, gcc has a long variant.  */
#    define topo_ffsl(x) __builtin_ffsl(x)
#  else
#    define topo_ffs(x) __builtin_ffs(x)
#    define TOPO_NEED_FFSL
#  endif

#elif defined(TOPO_HAVE_FFSL)

#  define topo_ffsl(x) ffsl(x)

#elif defined(TOPO_HAVE_FFS)

#  ifndef TOPO_HAVE_DECL_FFS
extern int ffs(int);
#  endif

#  define topo_ffs(x) ffs(x)
#  define TOPO_NEED_FFSL

#else /* no ffs implementation */

static __inline__ int topo_ffsl(unsigned long x)
{
	int i;

	if (!x)
		return 0;

	i = 1;
#ifdef TOPO_BITS_PER_LONG >= 64
	if (!(x & 0xfffffffful)) {
		x >>= 32;
		i += 32;
	}
#endif
	if (!(x & 0xffffu)) {
		x >>= 16;
		i += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		i += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		i += 4;
	}
	if (!(x & 0x3)) {
		x >>= 2;
		i += 2;
	}
	if (!(x & 0x1)) {
		x >>= 1;
		i += 1;
	}

	return i;
}

#endif

#ifdef TOPO_NEED_FFSL

/* We only have an int ffs(int) implementation, build a long one.  */

/* First make it 32 bits if it was only 16.  */
static __inline__ int topo_ffs32(unsigned long x)
{
#if TOPO_BITS_PER_INT == 16
	int low_ffs, hi_ffs;

	low_ffs = topo_ffs(x & 0xfffful);
	if (low_ffs)
		return low_ffs;

	hi_ffs = topo_ffs(x >> 16);
	if (hi_ffs)
		return hi_ffs + 16;

	return 0;
#else
	return topo_ffs(x);
#endif
}

/* Then make it 64 bit if longs are.  */
static __inline__ int topo_ffsl(unsigned long x)
{
#if TOPO_BITS_PER_LONG == 64
	int low_ffs, hi_ffs;

	low_ffs = topo_ffs32(x & 0xfffffffful);
	if (low_ffs)
		return low_ffs;

	hi_ffs = topo_ffs32(x >> 32);
	if (hi_ffs)
		return hi_ffs + 32;

	return 0;
#else
	return topo_ffs32(x);
#endif
}
#endif


static __inline__ int topo_weight_long(unsigned long w)
{
#if TOPO_BITS_PER_LONG == 32
#if (__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__) >= 4)
	return __builtin_popcount(w);
#else
	unsigned int res = (w & 0x55555555) + ((w >> 1) & 0x55555555);
	res = (res & 0x33333333) + ((res >> 2) & 0x33333333);
	res = (res & 0x0F0F0F0F) + ((res >> 4) & 0x0F0F0F0F);
	res = (res & 0x00FF00FF) + ((res >> 8) & 0x00FF00FF);
	return (res & 0x0000FFFF) + ((res >> 16) & 0x0000FFFF);
#endif
#else /* TOPO_BITS_PER_LONG == 32 */
#if (__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__) >= 4)
	return __builtin_popcountll(w);
#else
	unsigned long res;
	res = (w & 0x5555555555555555ul) + ((w >> 1) & 0x5555555555555555ul);
	res = (res & 0x3333333333333333ul) + ((res >> 2) & 0x3333333333333333ul);
	res = (res & 0x0F0F0F0F0F0F0F0Ful) + ((res >> 4) & 0x0F0F0F0F0F0F0F0Ful);
	res = (res & 0x00FF00FF00FF00FFul) + ((res >> 8) & 0x00FF00FF00FF00FFul);
	res = (res & 0x0000FFFF0000FFFFul) + ((res >> 16) & 0x0000FFFF0000FFFFul);
	return (res & 0x00000000FFFFFFFFul) + ((res >> 32) & 0x00000000FFFFFFFFul);
#endif
#endif /* TOPO_BITS_PER_LONG == 64 */
}


#endif /* TOPOLOGY_CPUSET_BITS_H */
