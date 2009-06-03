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

/** \file
 * \brief The cpuset API, for use in libtopology itself.
 */

#ifndef TOPOLOGY_CPUSET_H
#define TOPOLOGY_CPUSET_H

#include <topology/config.h>
#include <topology/cpuset-bits.h>


/** \defgroup topology_cpuset The cpuset API, for use in libtopology itself.
 * @{
 */


/**
 * Main Cpuset Type.
 */

typedef struct { unsigned long s[TOPO_CPUSUBSET_COUNT]; } topo_cpuset_t;



/**
 * Misc Predefined Cpuset Values.
 */
#define TOPO_CPUSET_ZERO	(topo_cpuset_t){ .s[0 ... TOPO_CPUSUBSET_COUNT-1] = TOPO_CPUSUBSET_ZERO }
#define TOPO_CPUSET_FULL	(topo_cpuset_t){ .s[0 ... TOPO_CPUSUBSET_COUNT-1] = TOPO_CPUSUBSET_FULL }
#define TOPO_CPUSET_CPU(cpu)	({ topo_cpuset_t __set = TOPO_CPUSET_ZERO; TOPO_CPUSUBSET_CPUSUBSET(__set,cpu) = TOPO_CPUSUBSET_VAL(cpu); __set; })


/**
 * Cpuset/String Conversion
 */

#define TOPO_CPUSET_STRING_LENGTH		(TOPO_CPUSUBSET_COUNT*(TOPO_CPUSUBSET_STRING_LENGTH+1))
#define TOPO_PRIxCPUSET		"s"

/** \brief Stringify a cpuset. */
static __inline__ int
topo_cpuset_snprintf(char * buf, size_t _size, topo_cpuset_t * set)
{
  ssize_t size = _size;
  char *tmp = buf;
  int res;
  int i;

  /* mark the end in case we do nothing later */
  *tmp = '\0';

  for(i=TOPO_CPUSUBSET_COUNT-1; i>=0; i--) {
    /* add a comma first, if we already wrote something  */
    if (tmp != buf) {
      res = snprintf(tmp, size, ",");
      tmp += res; size -= res;
      if (size <= 1) /* need room for ending \0 */
	break;
    }

    if (set->s[i] != 0)
      /* print the whole subset if not empty */
      res = snprintf(tmp, size, TOPO_PRIxCPUSUBSET, set->s[i]);
    else if (i == 0)
      /* print a single 0 to mark the last subset */
      res = snprintf(tmp, size, "0");
    else
      res = 0;

    tmp += res; size -= res;
    if (size <= 1) /* need room for ending \0 */
      break;
  }

  return tmp-buf+1;
}

/** \brief Parse a cpuset string.
 * Must start and end with a digit.
 */
static __inline__ void
topo_cpuset_from_string(const char * string, topo_cpuset_t * set)
{
  char * current = (char *) string;
  int count=0, i;

  while (*current != '\0') {
    unsigned long val;
    char *next;
    val = strtoul(current, &next, 16);
    /* store subset in order, starting from the end */
    set->s[TOPO_CPUSUBSET_COUNT-1-count] = val;
    count++;
    if (*next != ',')
      break;
    current = next+1;
    if (count == TOPO_CPUSUBSET_COUNT)
      break;
  }

  /* move subsets back to the beginning and clear the missing subsets */
  memmove(&set->s[0], &set->s[TOPO_CPUSUBSET_COUNT-count], count*sizeof(set->s[0]));
  for(i=count; i<TOPO_CPUSUBSET_COUNT; i++)
    set->s[i] = 0;
}



/**
 *  Primitives & macros for building, modifying and consulting "sets" of cpus.
 */

/** \brief Empty CPU set */
static __inline__ void topo_cpuset_zero(topo_cpuset_t * set);
static __inline__ void topo_cpuset_zero(topo_cpuset_t * set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) = TOPO_CPUSUBSET_ZERO;
}

/** \brief Fill CPU set */
static __inline__ void topo_cpuset_fill(topo_cpuset_t * set);
static __inline__ void topo_cpuset_fill(topo_cpuset_t * set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) = TOPO_CPUSUBSET_FULL;
}

/** \brief CPU set from an unsigned long mask */
static __inline__ void topo_cpuset_from_ulong(topo_cpuset_t *set, unsigned long mask);
static __inline__ void topo_cpuset_from_ulong(topo_cpuset_t *set, unsigned long mask)
{
	int i;
	TOPO_CPUSUBSET_SUBSET(*set,0) = mask;
	for(i=1; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) = TOPO_CPUSUBSET_ZERO;
}

/** \brief CPU set from an unsigned long mask, used as ith subset */
static __inline__ void topo_cpuset_from_ith_ulong(topo_cpuset_t *set, int i, unsigned long mask);
static __inline__ void topo_cpuset_from_ith_ulong(topo_cpuset_t *set, int i, unsigned long mask)
{
	int j;
	TOPO_CPUSUBSET_SUBSET(*set,i) = mask;
	for(j=1; j<TOPO_CPUSUBSET_COUNT; j++)
		if (j != i)
			TOPO_CPUSUBSET_SUBSET(*set,j) = TOPO_CPUSUBSET_ZERO;
}

/** \brief unsigned long mask from CPU set */
static __inline__ unsigned long topo_cpuset_to_ulong(const topo_cpuset_t *set);
static __inline__ unsigned long topo_cpuset_to_ulong(const topo_cpuset_t *set)
{
	return TOPO_CPUSUBSET_SUBSET(*set,0);
}

/** \brief unsigned long mask as ith subset from CPU set */
static __inline__ unsigned long topo_cpuset_to_ith_ulong(const topo_cpuset_t *set, int i);
static __inline__ unsigned long topo_cpuset_to_ith_ulong(const topo_cpuset_t *set, int i)
{
	return TOPO_CPUSUBSET_SUBSET(*set,i);
}

/** \brief Clear CPU set and set CPU \e cpu */
static __inline__ void topo_cpuset_cpu(topo_cpuset_t * set,
				       unsigned cpu);
static __inline__ void topo_cpuset_cpu(topo_cpuset_t * set,
				       unsigned cpu)
{
	topo_cpuset_zero(set);
	TOPO_CPUSUBSET_CPUSUBSET(*set,cpu) |= TOPO_CPUSUBSET_VAL(cpu);
}

/** \brief Clear CPU set and set all but the CPU \e cpu */
static __inline__ void topo_cpuset_all_but_cpu(topo_cpuset_t * set,
					       unsigned cpu);
static __inline__ void topo_cpuset_all_but_cpu(topo_cpuset_t * set,
					       unsigned cpu)
{
	topo_cpuset_fill(set);
	TOPO_CPUSUBSET_CPUSUBSET(*set,cpu) &= ~TOPO_CPUSUBSET_VAL(cpu);
}

/** \brief Add CPU \e cpu in CPU set \e set */
static __inline__ void topo_cpuset_set(topo_cpuset_t * set,
				       unsigned cpu);
static __inline__ void topo_cpuset_set(topo_cpuset_t * set,
				       unsigned cpu)
{
	TOPO_CPUSUBSET_CPUSUBSET(*set,cpu) |= TOPO_CPUSUBSET_VAL(cpu);
}

/** \brief Add CPUs from \e begincpu to \e endcpu in CPU set \e set */
static __inline__ void topo_cpuset_set_range(topo_cpuset_t * set,
					     unsigned begincpu, unsigned endcpu);
static __inline__ void topo_cpuset_set_range(topo_cpuset_t * set,
					     unsigned begincpu, unsigned endcpu)
{
	int i;
	for (i=begincpu; i<=endcpu; i++)
		TOPO_CPUSUBSET_CPUSUBSET(*set,i) |= TOPO_CPUSUBSET_VAL(i);
}

/** \brief Remove CPU \e cpu from CPU set \e set */
static __inline__ void topo_cpuset_clr(topo_cpuset_t * set,
				       unsigned cpu);
static __inline__ void topo_cpuset_clr(topo_cpuset_t * set,
				       unsigned cpu)
{
	TOPO_CPUSUBSET_CPUSUBSET(*set,cpu) &= ~TOPO_CPUSUBSET_VAL(cpu);
}

/** \brief Test whether CPU \e cpu is part of set \e set */
static __inline__ int topo_cpuset_isset(const topo_cpuset_t * set,
					unsigned cpu);
static __inline__ int topo_cpuset_isset(const topo_cpuset_t * set,
					unsigned cpu)
{
	return (TOPO_CPUSUBSET_CPUSUBSET(*set,cpu) & TOPO_CPUSUBSET_VAL(cpu)) != 0;
}

/** \brief Test whether set \e set is zero or full */
static __inline__ int topo_cpuset_iszero(const topo_cpuset_t *set);
static __inline__ int topo_cpuset_isfull(const topo_cpuset_t *set);
static __inline__ int topo_cpuset_iszero(const topo_cpuset_t *set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		if (TOPO_CPUSUBSET_SUBSET(*set,i) != TOPO_CPUSUBSET_ZERO)
			return 0;
	return 1;
}

static __inline__ int topo_cpuset_isfull(const topo_cpuset_t *set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		if (TOPO_CPUSUBSET_SUBSET(*set,i) != TOPO_CPUSUBSET_FULL)
			return 0;
	return 1;
}

/** \brief Test whether set \e set1 is equal to set \e set2 */
static __inline__ int topo_cpuset_isequal (const topo_cpuset_t *set1,
					 const topo_cpuset_t *set2);
static __inline__ int topo_cpuset_isequal (const topo_cpuset_t *set1,
					 const topo_cpuset_t *set2)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		if (TOPO_CPUSUBSET_SUBSET(*set1,i) != TOPO_CPUSUBSET_SUBSET(*set2,i))
			return 0;
	return 1;
}

/** \brief Test whether sets \e set1 and \e set2 intersects */
static __inline__ int topo_cpuset_intersects (const topo_cpuset_t *set1,
					      const topo_cpuset_t *set2);
static __inline__ int topo_cpuset_intersects (const topo_cpuset_t *set1,
					      const topo_cpuset_t *set2)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		if ((TOPO_CPUSUBSET_SUBSET(*set1,i) & TOPO_CPUSUBSET_SUBSET(*set2,i)) != TOPO_CPUSUBSET_ZERO)
			return 1;
	return 0;
}

/** \brief Test whether set \e sub_set is part of set \e super_set */
static __inline__ int topo_cpuset_isincluded (const topo_cpuset_t *sub_set,
					      const topo_cpuset_t *super_set);
static __inline__ int topo_cpuset_isincluded (const topo_cpuset_t *sub_set,
					      const topo_cpuset_t *super_set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		if (TOPO_CPUSUBSET_SUBSET(*super_set,i) != (TOPO_CPUSUBSET_SUBSET(*super_set,i) | TOPO_CPUSUBSET_SUBSET(*sub_set,i)))
			return 0;
	return 1;
}

/** \brief Or set \e modifier_set into set \e set */
static __inline__ void topo_cpuset_orset (topo_cpuset_t *set,
					  const topo_cpuset_t *modifier_set);
static __inline__ void topo_cpuset_orset (topo_cpuset_t *set,
					  const topo_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) |= TOPO_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief And set \e modifier_set into set \e set */
static __inline__ void topo_cpuset_andset (topo_cpuset_t *set,
					   const topo_cpuset_t *modifier_set);
static __inline__ void topo_cpuset_andset (topo_cpuset_t *set,
					   const topo_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) &= TOPO_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief Clear set \e modifier_set out of set \e set */
static __inline__ void topo_cpuset_clearset (topo_cpuset_t *set,
					     const topo_cpuset_t *modifier_set);
static __inline__ void topo_cpuset_clearset (topo_cpuset_t *set,
					     const topo_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) &= ~TOPO_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief Xor set \e set with set \e modifier_set */
static __inline__ void topo_cpuset_xorset (topo_cpuset_t *set,
					   const topo_cpuset_t *modifier_set);
static __inline__ void topo_cpuset_xorset (topo_cpuset_t *set,
					   const topo_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) ^= TOPO_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief Compute the first CPU (least significant bit) in CPU mask */
static __inline__ int topo_cpuset_first(const topo_cpuset_t * cpuset);
static __inline__ int topo_cpuset_first(const topo_cpuset_t * cpuset)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++) {
		/* subsets are unsigned longs, use ffsl */
		int _ffs = topo_ffsl(TOPO_CPUSUBSET_SUBSET(*cpuset,i));
		if (_ffs>0)
			return _ffs - 1 + TOPO_CPUSUBSET_SIZE*i;
	}

	return -1;
}

/** \brief Compar two cpusets using their first set bit.
 * Smaller least significant bit is smaller.
 * Empty cpuset are considered higher than anything.
 */
static __inline__ int topo_cpuset_compar_first(const topo_cpuset_t * set1,
					       const topo_cpuset_t * set2);
static __inline__ int topo_cpuset_compar_first(const topo_cpuset_t * set1,
					       const topo_cpuset_t * set2)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++) {
		int _ffs1 = topo_ffsl(TOPO_CPUSUBSET_SUBSET(*set1,i));
		int _ffs2 = topo_ffsl(TOPO_CPUSUBSET_SUBSET(*set2,i));
		if (!_ffs1 && !_ffs2)
			continue;
		/* if both have a bit set, compar for real */
		if (_ffs1 && _ffs2)
			return _ffs1-_ffs2;
		/* one is empty, and it is considered higher, so reverse-compar them */
		return _ffs2-_ffs1;
	}
	return 0;	
}

/** \brief Compar two cpusets using their last bits.
 * Higher most significant bit is higher.
 * Empty cpuset are considered lower than anything.
 */
static __inline__ int topo_cpuset_compar(const topo_cpuset_t * set1,
					 const topo_cpuset_t * set2);
static __inline__ int topo_cpuset_compar(const topo_cpuset_t * set1,
					 const topo_cpuset_t * set2)
{
	int i;
	for(i=TOPO_CPUSUBSET_COUNT-1; i>=0; i--) {
		if (TOPO_CPUSUBSET_SUBSET(*set1,i) == TOPO_CPUSUBSET_SUBSET(*set2,i))
			continue;
		return TOPO_CPUSUBSET_SUBSET(*set1,i) < TOPO_CPUSUBSET_SUBSET(*set2,i) ? -1 : 1;
	}
	return 0;	
}

static inline int topo_weight_long(unsigned long w)
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

/** \brief Compute the weight of a CPU mask */
static __inline__ int topo_cpuset_weight(const topo_cpuset_t * cpuset);
static __inline__ int topo_cpuset_weight(const topo_cpuset_t * cpuset)
{
	int weight = 0;
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		weight += topo_weight_long(TOPO_CPUSUBSET_SUBSET(*cpuset,i));
	return weight;
}

/** \brief Loop macro iterating on a cpuset and yielding on each cpu that
 *  is member of the set.
 *  Uses variables \e set (the cpu set) and \e cpu (the loop variable) */
#define topo_cpuset_foreach_begin(cpu, cpuset) \
        for (cpu = 0; cpu < TOPO_NBMAXCPUS; cpu++) \
                if (topo_cpuset_isset(cpuset, cpu)) {
#define topo_cpuset_foreach_end() \
                }

/** @} */

#endif /* TOPOLOGY_CPUSET_H */
