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
 * \brief The Cpuset API, for use in libtopology itself.
 */

#ifndef TOPOLOGY_CPUSET_H
#define TOPOLOGY_CPUSET_H

#include <topology/config.h>
#include <topology/cpuset-bits.h>


/** \defgroup topology_cpuset The Cpuset API
 *
 * for use in libtopology itself.
 * @{
 */


/** \brief
 * Set of CPUs represented as a bitmask.
 */
typedef struct { unsigned long s[TOPO_CPUSUBSET_COUNT]; } topo_cpuset_t;



/*
 * Misc Predefined Cpuset Values.
 */

/** \brief Predefined cpuset with no CPU set */
#define TOPO_CPUSET_ZERO	(topo_cpuset_t){ .s[0 ... TOPO_CPUSUBSET_COUNT-1] = TOPO_CPUSUBSET_ZERO }
/** \brief Predefined cpuset with all CPUs set */
#define TOPO_CPUSET_FULL	(topo_cpuset_t){ .s[0 ... TOPO_CPUSUBSET_COUNT-1] = TOPO_CPUSUBSET_FULL }
/** \brief Predefined cpuset with CPU \c cpu set */
#define TOPO_CPUSET_CPU(cpu)	({ topo_cpuset_t __set = TOPO_CPUSET_ZERO; TOPO_CPUSUBSET_CPUSUBSET(__set,cpu) = TOPO_CPUSUBSET_VAL(cpu); __set; })


/*
 * Cpuset/String Conversion
 */

/** \brief Maximal required length of a string for printing a CPU set.
 *
 * Fewer characters may be needed if part of the CPU set is empty.
 */
#define TOPO_CPUSET_STRING_LENGTH		(TOPO_CPUSET_SUBSTRING_COUNT*(TOPO_CPUSET_SUBSTRING_LENGTH+1))
/** \brief Printf format for printing a CPU set */
#define TOPO_PRIxCPUSET		"s"

/** \brief Stringify a cpuset.
 *
 * Up to \p buflen characters may be written in buffer \p buf.
 * 
 * \return the number of character that were actually written (not including the ending \\0).
 */
static __inline__ int
topo_cpuset_snprintf(char * __topo_restrict buf, size_t buflen, const topo_cpuset_t * __topo_restrict set)
{
  ssize_t size = buflen;
  char *tmp = buf;
  int res;
  int i;
  unsigned long accum = 0;
  int accumed = 0;
#if TOPO_BITS_PER_LONG == TOPO_CPUSET_SUBSTRING_SIZE
  const unsigned long accum_mask = ~0UL;
#else /* TOPO_BITS_PER_LONG != TOPO_CPUSET_SUBSTRING_SIZE */
  const unsigned long accum_mask = ((1UL << TOPO_CPUSET_SUBSTRING_SIZE) - 1) << (TOPO_BITS_PER_LONG - TOPO_CPUSET_SUBSTRING_SIZE);
#endif /* TOPO_BITS_PER_LONG != TOPO_CPUSET_SUBSTRING_SIZE */

  /* mark the end in case we do nothing later */
  *tmp = '\0';

  i=TOPO_CPUSUBSET_COUNT-1;
  while (i>=0 || accumed) {
    /* add a comma first, if we already wrote something  */
    if (tmp != buf) {
      res = snprintf(tmp, size, ",");
      tmp += res; size -= res;
      if (size <= 1) /* need room for ending \0 */
	break;
    }

    /* Refill accumulator */
    if (!accumed) {
      accum = set->s[i--];
      accumed = TOPO_BITS_PER_LONG;
    }

    if (accum & accum_mask) {
      /* print the whole subset if not empty */
      res = snprintf(tmp, size, TOPO_PRIxCPUSUBSET, (accum & accum_mask) >> (TOPO_BITS_PER_LONG - TOPO_CPUSET_SUBSTRING_SIZE));
    } else if (i == -1 && accumed == TOPO_CPUSET_SUBSTRING_SIZE)
      /* print a single 0 to mark the last subset */
      res = snprintf(tmp, size, "0");
    else
      res = 0;
    accum <<= TOPO_CPUSET_SUBSTRING_SIZE;
    accumed -= TOPO_CPUSET_SUBSTRING_SIZE;

    tmp += res; size -= res;
    if (size <= 1) /* need room for ending \0 */
      break;
  }

  return tmp-buf+1;
}

/** \brief Parse a cpuset string.
 *
 * Must start and end with a digit.
 */
static __inline__ void
topo_cpuset_from_string(const char * __topo_restrict string, topo_cpuset_t * __topo_restrict set)
{
  char * current = (char *) string;
  int count=0, i;
  unsigned long accum = 0;
  int accumed = 0;

  while (*current != '\0') {
    unsigned long val;
    char *next;
    val = strtoul(current, &next, 16);
    /* store subset in order, starting from the end */
    accum = (accum << TOPO_CPUSET_SUBSTRING_SIZE) | val;
    accumed += TOPO_CPUSET_SUBSTRING_SIZE;
    if (accumed == TOPO_BITS_PER_LONG) {
      set->s[TOPO_CPUSUBSET_COUNT-1-count] = accum;
      count++;
      accum = 0;
      accumed = 0;
    }
    if (*next != ',')
      break;
    current = next+1;
    if (count == TOPO_CPUSUBSET_COUNT)
      break;
  }

  /* move subsets back to the beginning and clear the missing subsets */
  for (i = 0; i < count; i++) {
    set->s[i] = accum;
    set->s[i] |= set->s[TOPO_CPUSUBSET_COUNT-count+i] << accumed;
    accum = set->s[TOPO_CPUSUBSET_COUNT-count+i] >> accumed;
  }
  if (accumed && count < TOPO_CPUSUBSET_COUNT)
    set->s[i++] = accum;
  for( ; i<TOPO_CPUSUBSET_COUNT; i++)
    set->s[i] = 0;
}



/** \brief
 *  Primitives & macros for building, modifying and consulting "sets" of cpus.
 */

/** \brief Empty CPU set \p set */
static __inline__ void topo_cpuset_zero(topo_cpuset_t * set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) = TOPO_CPUSUBSET_ZERO;
}

/** \brief Fill CPU set \p set */
static __inline__ void topo_cpuset_fill(topo_cpuset_t * set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) = TOPO_CPUSUBSET_FULL;
}

/** \brief Setup CPU set \p set from unsigned long \p mask */
static __inline__ void topo_cpuset_from_ulong(topo_cpuset_t *set, unsigned long mask)
{
	int i;
	TOPO_CPUSUBSET_SUBSET(*set,0) = mask;
	for(i=1; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) = TOPO_CPUSUBSET_ZERO;
}

/** \brief Setup CPU set \p set from unsigned long \p mask used as \p i -th subset */
static __inline__ void topo_cpuset_from_ith_ulong(topo_cpuset_t *set, int i, unsigned long mask)
{
	int j;
	TOPO_CPUSUBSET_SUBSET(*set,i) = mask;
	for(j=1; j<TOPO_CPUSUBSET_COUNT; j++)
		if (j != i)
			TOPO_CPUSUBSET_SUBSET(*set,j) = TOPO_CPUSUBSET_ZERO;
}

/** \brief Convert the beginning part of CPU set \p set into unsigned long \p mask */
static __inline__ unsigned long topo_cpuset_to_ulong(const topo_cpuset_t *set)
{
	return TOPO_CPUSUBSET_SUBSET(*set,0);
}

/** \brief Convert the \p i -th subset of CPU set \p set into unsigned long mask */
static __inline__ unsigned long topo_cpuset_to_ith_ulong(const topo_cpuset_t *set, int i)
{
	return TOPO_CPUSUBSET_SUBSET(*set,i);
}

/** \brief Clear CPU set \p set and set CPU \p cpu */
static __inline__ void topo_cpuset_cpu(topo_cpuset_t * set,
				       unsigned cpu)
{
	topo_cpuset_zero(set);
	TOPO_CPUSUBSET_CPUSUBSET(*set,cpu) |= TOPO_CPUSUBSET_VAL(cpu);
}

/** \brief Clear CPU set \p set and set all but the CPU \p cpu */
static __inline__ void topo_cpuset_all_but_cpu(topo_cpuset_t * set,
					       unsigned cpu)
{
	topo_cpuset_fill(set);
	TOPO_CPUSUBSET_CPUSUBSET(*set,cpu) &= ~TOPO_CPUSUBSET_VAL(cpu);
}

/** \brief Add CPU \p cpu in CPU set \p set */
static __inline__ void topo_cpuset_set(topo_cpuset_t * set,
				       unsigned cpu)
{
	TOPO_CPUSUBSET_CPUSUBSET(*set,cpu) |= TOPO_CPUSUBSET_VAL(cpu);
}

/** \brief Add CPUs from \p begincpu to \p endcpu in CPU set \p set */
static __inline__ void topo_cpuset_set_range(topo_cpuset_t * set,
					     unsigned begincpu, unsigned endcpu)
{
	int i;
	for (i=begincpu; i<=endcpu; i++)
		TOPO_CPUSUBSET_CPUSUBSET(*set,i) |= TOPO_CPUSUBSET_VAL(i);
}

/** \brief Remove CPU \p cpu from CPU set \p set */
static __inline__ void topo_cpuset_clr(topo_cpuset_t * set,
				       unsigned cpu)
{
	TOPO_CPUSUBSET_CPUSUBSET(*set,cpu) &= ~TOPO_CPUSUBSET_VAL(cpu);
}

/** \brief Test whether CPU \p cpu is part of set \p set */
static __inline__ int topo_cpuset_isset(const topo_cpuset_t * set,
					unsigned cpu)
{
	return (TOPO_CPUSUBSET_CPUSUBSET(*set,cpu) & TOPO_CPUSUBSET_VAL(cpu)) != 0;
}

/** \brief Test whether set \p set is zero */
static __inline__ int topo_cpuset_iszero(const topo_cpuset_t *set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		if (TOPO_CPUSUBSET_SUBSET(*set,i) != TOPO_CPUSUBSET_ZERO)
			return 0;
	return 1;
}

/** \brief Test whether set \p set is full */
static __inline__ int topo_cpuset_isfull(const topo_cpuset_t *set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		if (TOPO_CPUSUBSET_SUBSET(*set,i) != TOPO_CPUSUBSET_FULL)
			return 0;
	return 1;
}

/** \brief Test whether set \p set1 is equal to set \p set2 */
static __inline__ int topo_cpuset_isequal (const topo_cpuset_t *set1,
					   const topo_cpuset_t *set2)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		if (TOPO_CPUSUBSET_SUBSET(*set1,i) != TOPO_CPUSUBSET_SUBSET(*set2,i))
			return 0;
	return 1;
}

/** \brief Test whether sets \p set1 and \p set2 intersects */
static __inline__ int topo_cpuset_intersects (const topo_cpuset_t *set1,
					      const topo_cpuset_t *set2)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		if ((TOPO_CPUSUBSET_SUBSET(*set1,i) & TOPO_CPUSUBSET_SUBSET(*set2,i)) != TOPO_CPUSUBSET_ZERO)
			return 1;
	return 0;
}

/** \brief Test whether set \p sub_set is part of set \p super_set */
static __inline__ int topo_cpuset_isincluded (const topo_cpuset_t *sub_set,
					      const topo_cpuset_t *super_set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		if (TOPO_CPUSUBSET_SUBSET(*super_set,i) != (TOPO_CPUSUBSET_SUBSET(*super_set,i) | TOPO_CPUSUBSET_SUBSET(*sub_set,i)))
			return 0;
	return 1;
}

/** \brief Or set \p modifier_set into set \p set */
static __inline__ void topo_cpuset_orset (topo_cpuset_t *set,
					  const topo_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) |= TOPO_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief And set \p modifier_set into set \p set */
static __inline__ void topo_cpuset_andset (topo_cpuset_t *set,
					   const topo_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) &= TOPO_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief Clear set \p modifier_set out of set \p set */
static __inline__ void topo_cpuset_clearset (topo_cpuset_t *set,
					     const topo_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) &= ~TOPO_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief Xor set \p set with set \p modifier_set */
static __inline__ void topo_cpuset_xorset (topo_cpuset_t *set,
					   const topo_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		TOPO_CPUSUBSET_SUBSET(*set,i) ^= TOPO_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief Compute the first CPU (least significant bit) in CPU set \p set */
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

/** \brief Keep a single CPU among those set in CPU set \p set
 *
 * Might be used before binding so that the process does not
 * have a chance of migrating between multiple logical CPUs
 * in the original mask.
 */
static __inline__ void topo_cpuset_singlify(topo_cpuset_t * set)
{
	int i,found = 0;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++) {
		if (found) {
			TOPO_CPUSUBSET_SUBSET(*set,i) = TOPO_CPUSUBSET_ZERO;
			continue;
		} else {
			/* subsets are unsigned longs, use ffsl */
			int _ffs = topo_ffsl(TOPO_CPUSUBSET_SUBSET(*set,i));
			if (_ffs>0) {
				TOPO_CPUSUBSET_SUBSET(*set,i) = TOPO_CPUSUBSET_VAL(_ffs-1);
				found = 1;
			}
		}
	}
}

/** \brief Compar CPU sets \p set1 and \p set2 using their first set bit.
 *
 * Smaller least significant bit is smaller.
 * The empty CPU set is considered higher than anything.
 */
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

/** \brief Compar CPU sets \p set1 and \p set2 using their last bits.
 *
 * Higher most significant bit is higher.
 * The empty CPU set is considered lower than anything.
 */
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

/** \brief Compute the weight of CPU set \p set */
static __inline__ int topo_cpuset_weight(const topo_cpuset_t * set)
{
	int weight = 0;
	int i;
	for(i=0; i<TOPO_CPUSUBSET_COUNT; i++)
		weight += topo_weight_long(TOPO_CPUSUBSET_SUBSET(*set,i));
	return weight;
}

/** \brief Loop macro iterating on CPU set \p set
 *
 * It yields on each cpu that is member of the set. It uses variables \p set
 * (the cpu set) and \p cpu (the loop variable)
 */
#define topo_cpuset_foreach_begin(cpu, set) \
        for (cpu = 0; cpu < TOPO_NBMAXCPUS; cpu++) \
                if (topo_cpuset_isset(set, cpu)) {
/** \brief End of loop
 *
 * \sa topo_cpuset_foreach_begin */
#define topo_cpuset_foreach_end() \
                }

/** @} */

#endif /* TOPOLOGY_CPUSET_H */
