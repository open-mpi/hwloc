/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * See COPYING in top-level directory.
 */

/** \file
 * \brief The Cpuset API, for use in hwloc itself.
 */

#ifndef HWLOC_CPUSET_H
#define HWLOC_CPUSET_H

#include <hwloc/config.h>
#include <hwloc/cpuset-bits.h>


/** \defgroup hwlocality_cpuset The Cpuset API
 *
 * For use in hwloc itself, a hwloc_cpuset_t represents a set of logical
 * processors.
 *
 * \note cpusets are indexed by OS logical processor number.
 * @{
 */


/** \brief
 * Set of CPUs represented as a bitmask.
 */
typedef struct { unsigned long s[HWLOC_CPUSUBSET_COUNT]; } hwloc_cpuset_t;


/*
 * Misc Predefined Cpuset Values.
 */

/*
 * Cpuset/String Conversion
 */

/** \brief Maximal required length of a string for printing a CPU set.
 *
 * Fewer characters may be needed if part of the CPU set is empty.
 */
#define HWLOC_CPUSET_STRING_LENGTH		(HWLOC_CPUSET_SUBSTRING_COUNT*(HWLOC_CPUSET_SUBSTRING_LENGTH+1))
/** \brief Printf format for printing a CPU set */
#define HWLOC_PRIxCPUSET		"s"

/** \brief Stringify a cpuset.
 *
 * Up to \p buflen characters may be written in buffer \p buf.
 *
 * \return the number of character that were actually written if not truncating,
 * or that would have been written  (not including the ending \\0).
 */
static __inline int
hwloc_cpuset_snprintf(char * __hwloc_restrict buf, size_t buflen, const hwloc_cpuset_t * __hwloc_restrict set)
{
  ssize_t size = buflen;
  char *tmp = buf;
  int res;
  int needcomma = 0;
  int missed = 0;
  int i;
  unsigned long accum = 0;
  int accumed = 0;
#if HWLOC_BITS_PER_LONG == HWLOC_CPUSET_SUBSTRING_SIZE
  const unsigned long accum_mask = ~0UL;
#else /* HWLOC_BITS_PER_LONG != HWLOC_CPUSET_SUBSTRING_SIZE */
  const unsigned long accum_mask = ((1UL << HWLOC_CPUSET_SUBSTRING_SIZE) - 1) << (HWLOC_BITS_PER_LONG - HWLOC_CPUSET_SUBSTRING_SIZE);
#endif /* HWLOC_BITS_PER_LONG != HWLOC_CPUSET_SUBSTRING_SIZE */

  /* mark the end in case we do nothing later */
  if (buflen > 0)
    tmp[0] = '\0';

  i=HWLOC_CPUSUBSET_COUNT-1;
  while (i>=0 || accumed) {
    /* Refill accumulator */
    if (!accumed) {
      accum = set->s[i--];
      accumed = HWLOC_BITS_PER_LONG;
    }

    if (accum & accum_mask) {
      /* print the whole subset if not empty */
      res = snprintf(tmp, size, needcomma ? "," HWLOC_PRIxCPUSUBSET : HWLOC_PRIxCPUSUBSET,
		     (accum & accum_mask) >> (HWLOC_BITS_PER_LONG - HWLOC_CPUSET_SUBSTRING_SIZE));
      needcomma = 1;
    } else if (i == -1 && accumed == HWLOC_CPUSET_SUBSTRING_SIZE) {
      /* print a single 0 to mark the last subset */
      res = snprintf(tmp, size, needcomma ? ",0" : "0");
    } else if (needcomma) {
      res = snprintf(tmp, size, ",");
    } else {
      res = 0;
    }

#if HWLOC_BITS_PER_LONG == HWLOC_CPUSET_SUBSTRING_SIZE
    accum = 0;
    accumed = 0;
#else
    accum <<= HWLOC_CPUSET_SUBSTRING_SIZE;
    accumed -= HWLOC_CPUSET_SUBSTRING_SIZE;
#endif

    if (res >= size) {
      int written = size>0 ? size-1 : 0;
      missed += res - written;
      res = written;
    }
    tmp += res;
    size -= res;
  }

  return tmp-buf+missed;
}

/** \brief Parse a cpuset string.
 *
 * Must start and end with a digit.
 */
static __inline void
hwloc_cpuset_from_string(const char * __hwloc_restrict string, hwloc_cpuset_t * __hwloc_restrict set)
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
#if HWLOC_BITS_PER_LONG == HWLOC_CPUSET_SUBSTRING_SIZE
    accum = val;
#else
    accum = (accum << HWLOC_CPUSET_SUBSTRING_SIZE) | val;
#endif
    accumed += HWLOC_CPUSET_SUBSTRING_SIZE;
    if (accumed == HWLOC_BITS_PER_LONG) {
      set->s[HWLOC_CPUSUBSET_COUNT-1-count] = accum;
      count++;
      accum = 0;
      accumed = 0;
    }
    if (*next != ',')
      break;
    current = next+1;
    if (count == HWLOC_CPUSUBSET_COUNT)
      break;
  }

  /* move subsets back to the beginning and clear the missing subsets */
  for (i = 0; i < count; i++) {
    set->s[i] = accum;
    set->s[i] |= set->s[HWLOC_CPUSUBSET_COUNT-count+i] << accumed;
    if (accumed)
      accum = set->s[HWLOC_CPUSUBSET_COUNT-count+i] >> (HWLOC_BITS_PER_LONG - accumed);
  }
  /* Remaining bit from last iteration */
  if (accumed && count < HWLOC_CPUSUBSET_COUNT)
    set->s[i++] = accum;
  for( ; i<HWLOC_CPUSUBSET_COUNT; i++)
    set->s[i] = 0;
}



/** \brief
 *  Primitives & macros for building, modifying and consulting "sets" of cpus.
 */

/** \brief Empty CPU set \p set */
static __inline void hwloc_cpuset_zero(hwloc_cpuset_t * set)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_ZERO;
}

/** \brief Fill CPU set \p set */
static __inline void hwloc_cpuset_fill(hwloc_cpuset_t * set)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_FULL;
}

/** \brief Setup CPU set \p set from unsigned long \p mask */
static __inline void hwloc_cpuset_from_ulong(hwloc_cpuset_t *set, unsigned long mask)
{
	int i;
	HWLOC_CPUSUBSET_SUBSET(*set,0) = mask;
	for(i=1; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_ZERO;
}

/** \brief Setup CPU set \p set from unsigned long \p mask used as \p i -th subset */
static __inline void hwloc_cpuset_from_ith_ulong(hwloc_cpuset_t *set, int i, unsigned long mask)
{
	int j;
	HWLOC_CPUSUBSET_SUBSET(*set,i) = mask;
	for(j=1; j<HWLOC_CPUSUBSET_COUNT; j++)
		if (j != i)
			HWLOC_CPUSUBSET_SUBSET(*set,j) = HWLOC_CPUSUBSET_ZERO;
}

/** \brief Convert the beginning part of CPU set \p set into unsigned long \p mask */
static __inline unsigned long hwloc_cpuset_to_ulong(const hwloc_cpuset_t *set)
{
	return HWLOC_CPUSUBSET_SUBSET(*set,0);
}

/** \brief Convert the \p i -th subset of CPU set \p set into unsigned long mask */
static __inline unsigned long hwloc_cpuset_to_ith_ulong(const hwloc_cpuset_t *set, int i)
{
	return HWLOC_CPUSUBSET_SUBSET(*set,i);
}

/** \brief Clear CPU set \p set and set CPU \p cpu */
static __inline void hwloc_cpuset_cpu(hwloc_cpuset_t * set,
				       unsigned cpu)
{
	hwloc_cpuset_zero(set);
	HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) |= HWLOC_CPUSUBSET_VAL(cpu);
}

/** \brief Clear CPU set \p set and set all but the CPU \p cpu */
static __inline void hwloc_cpuset_all_but_cpu(hwloc_cpuset_t * set,
					       unsigned cpu)
{
	hwloc_cpuset_fill(set);
	HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) &= ~HWLOC_CPUSUBSET_VAL(cpu);
}

/** \brief Add CPU \p cpu in CPU set \p set */
static __inline void hwloc_cpuset_set(hwloc_cpuset_t * set,
				       unsigned cpu)
{
	HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) |= HWLOC_CPUSUBSET_VAL(cpu);
}

/** \brief Add CPUs from \p begincpu to \p endcpu in CPU set \p set */
static __inline void hwloc_cpuset_set_range(hwloc_cpuset_t * set,
					     unsigned begincpu, unsigned endcpu)
{
	int i;
	for (i=begincpu; i<=endcpu; i++)
		HWLOC_CPUSUBSET_CPUSUBSET(*set,i) |= HWLOC_CPUSUBSET_VAL(i);
}

/** \brief Remove CPU \p cpu from CPU set \p set */
static __inline void hwloc_cpuset_clr(hwloc_cpuset_t * set,
				       unsigned cpu)
{
	HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) &= ~HWLOC_CPUSUBSET_VAL(cpu);
}

/** \brief Test whether CPU \p cpu is part of set \p set */
static __inline int hwloc_cpuset_isset(const hwloc_cpuset_t * set,
					unsigned cpu)
{
	return (HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) & HWLOC_CPUSUBSET_VAL(cpu)) != 0;
}

/** \brief Test whether set \p set is zero */
static __inline int hwloc_cpuset_iszero(const hwloc_cpuset_t *set)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*set,i) != HWLOC_CPUSUBSET_ZERO)
			return 0;
	return 1;
}

/** \brief Test whether set \p set is full */
static __inline int hwloc_cpuset_isfull(const hwloc_cpuset_t *set)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*set,i) != HWLOC_CPUSUBSET_FULL)
			return 0;
	return 1;
}

/** \brief Test whether set \p set1 is equal to set \p set2 */
static __inline int hwloc_cpuset_isequal (const hwloc_cpuset_t *set1,
					   const hwloc_cpuset_t *set2)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*set1,i) != HWLOC_CPUSUBSET_SUBSET(*set2,i))
			return 0;
	return 1;
}

/** \brief Test whether sets \p set1 and \p set2 intersects */
static __inline int hwloc_cpuset_intersects (const hwloc_cpuset_t *set1,
					      const hwloc_cpuset_t *set2)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		if ((HWLOC_CPUSUBSET_SUBSET(*set1,i) & HWLOC_CPUSUBSET_SUBSET(*set2,i)) != HWLOC_CPUSUBSET_ZERO)
			return 1;
	return 0;
}

/** \brief Test whether set \p sub_set is part of set \p super_set */
static __inline int hwloc_cpuset_isincluded (const hwloc_cpuset_t *sub_set,
					      const hwloc_cpuset_t *super_set)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*super_set,i) != (HWLOC_CPUSUBSET_SUBSET(*super_set,i) | HWLOC_CPUSUBSET_SUBSET(*sub_set,i)))
			return 0;
	return 1;
}

/** \brief Or set \p modifier_set into set \p set */
static __inline void hwloc_cpuset_orset (hwloc_cpuset_t *set,
					  const hwloc_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) |= HWLOC_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief And set \p modifier_set into set \p set */
static __inline void hwloc_cpuset_andset (hwloc_cpuset_t *set,
					   const hwloc_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) &= HWLOC_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief Clear set \p modifier_set out of set \p set */
static __inline void hwloc_cpuset_clearset (hwloc_cpuset_t *set,
					     const hwloc_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) &= ~HWLOC_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief Xor set \p set with set \p modifier_set */
static __inline void hwloc_cpuset_xorset (hwloc_cpuset_t *set,
					   const hwloc_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) ^= HWLOC_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief Compute the first CPU (least significant bit) in CPU set \p set */
static __inline int hwloc_cpuset_first(const hwloc_cpuset_t * cpuset)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++) {
		/* subsets are unsigned longs, use ffsl */
		int _ffs = hwloc_ffsl(HWLOC_CPUSUBSET_SUBSET(*cpuset,i));
		if (_ffs>0)
			return _ffs - 1 + HWLOC_CPUSUBSET_SIZE*i;
	}

	return -1;
}

/** \brief Compute the last CPU (most significant bit) in CPU set \p set */
static __inline int hwloc_cpuset_last(const hwloc_cpuset_t * cpuset)
{
	int i;
	for(i=HWLOC_CPUSUBSET_COUNT-1; i>=0; i--) {
		/* subsets are unsigned longs, use flsl */
		int _fls = hwloc_flsl(HWLOC_CPUSUBSET_SUBSET(*cpuset,i));
		if (_fls>0)
			return _fls - 1 + HWLOC_CPUSUBSET_SIZE*i;
	}

	return -1;
}

/** \brief Keep a single CPU among those set in CPU set \p set
 *
 * Might be used before binding so that the process does not
 * have a chance of migrating between multiple logical CPUs
 * in the original mask.
 */
static __inline void hwloc_cpuset_singlify(hwloc_cpuset_t * set)
{
	int i,found = 0;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++) {
		if (found) {
			HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_ZERO;
			continue;
		} else {
			/* subsets are unsigned longs, use ffsl */
			int _ffs = hwloc_ffsl(HWLOC_CPUSUBSET_SUBSET(*set,i));
			if (_ffs>0) {
				HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_VAL(_ffs-1);
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
static __inline int hwloc_cpuset_compar_first(const hwloc_cpuset_t * set1,
					       const hwloc_cpuset_t * set2)
{
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++) {
		int _ffs1 = hwloc_ffsl(HWLOC_CPUSUBSET_SUBSET(*set1,i));
		int _ffs2 = hwloc_ffsl(HWLOC_CPUSUBSET_SUBSET(*set2,i));
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
static __inline int hwloc_cpuset_compar(const hwloc_cpuset_t * set1,
					 const hwloc_cpuset_t * set2)
{
	int i;
	for(i=HWLOC_CPUSUBSET_COUNT-1; i>=0; i--) {
		if (HWLOC_CPUSUBSET_SUBSET(*set1,i) == HWLOC_CPUSUBSET_SUBSET(*set2,i))
			continue;
		return HWLOC_CPUSUBSET_SUBSET(*set1,i) < HWLOC_CPUSUBSET_SUBSET(*set2,i) ? -1 : 1;
	}
	return 0;
}

/** \brief Compute the weight of CPU set \p set */
static __inline int hwloc_cpuset_weight(const hwloc_cpuset_t * set)
{
	int weight = 0;
	int i;
	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		weight += hwloc_weight_long(HWLOC_CPUSUBSET_SUBSET(*set,i));
	return weight;
}

/** \brief Loop macro iterating on CPU set \p set
 *
 * It yields on each cpu that is member of the set. It uses variables \p set
 * (the cpu set) and \p cpu (the loop variable)
 */
#define hwloc_cpuset_foreach_begin(cpu, set) \
        for (cpu = 0; cpu < HWLOC_NBMAXCPUS; cpu++) \
                if (hwloc_cpuset_isset(set, cpu)) {
/** \brief End of loop
 *
 * \sa hwloc_cpuset_foreach_begin */
#define hwloc_cpuset_foreach_end() \
                }

/** @} */

#endif /* HWLOC_CPUSET_H */
