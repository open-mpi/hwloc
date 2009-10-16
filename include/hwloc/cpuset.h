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


/** \defgroup hwlocality_cpuset The Cpuset API
 *
 * For use in hwloc itself, a hwloc_cpuset_t represents a set of logical
 * processors.
 *
 * \note cpusets are indexed by OS logical processor number.
 * @{
 */


/** \brief
 * Set of CPUs represented as an opaque pointer to an internal bitmask.
 */
typedef struct hwloc_cpuset_s * hwloc_cpuset_t;
typedef const struct hwloc_cpuset_s * hwloc_const_cpuset_t;


/*
 * CPU set allocation, freeing and copying.
 */

/** \brief Allocate a new empty CPU set */
hwloc_cpuset_t hwloc_cpuset_alloc(void);

/** \brief Free CPU set \p set */
void hwloc_cpuset_free(hwloc_cpuset_t set);

/** \brief Duplicate CPU set \p set by allocating a new CPU set and copying its contents */
hwloc_cpuset_t hwloc_cpuset_dup(hwloc_cpuset_t set);

/** \brief Copy the contents of CPU set \p src into the already allocated CPU set \p dst */
void hwloc_cpuset_copy(hwloc_cpuset_t dst, hwloc_cpuset_t src);


/*
 * Cpuset/String Conversion
 */

/** \brief Stringify a cpuset.
 *
 * Up to \p buflen characters may be written in buffer \p buf.
 *
 * \return the number of character that were actually written if not truncating,
 * or that would have been written  (not including the ending \\0).
 */
int hwloc_cpuset_snprintf(char * __hwloc_restrict buf, size_t buflen, hwloc_const_cpuset_t set);

/** \brief Stringify a cpuset into a newly allocated string.
 *
 * \return the number of character that were actually written
 * (not including the ending \\0).
 */
int hwloc_cpuset_asprintf(char ** strp, hwloc_const_cpuset_t set);

/** \brief Parse a cpuset string.
 *
 * Must start and end with a digit.
 */
hwloc_cpuset_t hwloc_cpuset_from_string(const char * __hwloc_restrict string);


/** \brief
 *  Primitives & macros for building, modifying and consulting "sets" of cpus.
 */

/** \brief Empty CPU set \p set */
void hwloc_cpuset_zero(hwloc_cpuset_t set);

/** \brief Fill CPU set \p set */
void hwloc_cpuset_fill(hwloc_cpuset_t set);

/** \brief Setup CPU set \p set from unsigned long \p mask */
void hwloc_cpuset_from_ulong(hwloc_cpuset_t set, unsigned long mask);

/** \brief Setup CPU set \p set from unsigned long \p mask used as \p i -th subset */
void hwloc_cpuset_from_ith_ulong(hwloc_cpuset_t set, int i, unsigned long mask);

/** \brief Convert the beginning part of CPU set \p set into unsigned long \p mask */
unsigned long hwloc_cpuset_to_ulong(hwloc_const_cpuset_t set);

/** \brief Convert the \p i -th subset of CPU set \p set into unsigned long mask */
unsigned long hwloc_cpuset_to_ith_ulong(hwloc_const_cpuset_t set, int i);

/** \brief Clear CPU set \p set and set CPU \p cpu */
void hwloc_cpuset_cpu(hwloc_cpuset_t set, unsigned cpu);

/** \brief Clear CPU set \p set and set all but the CPU \p cpu */
void hwloc_cpuset_all_but_cpu(hwloc_cpuset_t set, unsigned cpu);

/** \brief Add CPU \p cpu in CPU set \p set */
void hwloc_cpuset_set(hwloc_cpuset_t set, unsigned cpu);

/** \brief Add CPUs from \p begincpu to \p endcpu in CPU set \p set */
void hwloc_cpuset_set_range(hwloc_cpuset_t set, unsigned begincpu, unsigned endcpu);

/** \brief Remove CPU \p cpu from CPU set \p set */
void hwloc_cpuset_clr(hwloc_cpuset_t set, unsigned cpu);

/** \brief Test whether CPU \p cpu is part of set \p set */
int hwloc_cpuset_isset(hwloc_const_cpuset_t set, unsigned cpu);

/** \brief Test whether set \p set is zero */
int hwloc_cpuset_iszero(hwloc_const_cpuset_t set);

/** \brief Test whether set \p set is full */
int hwloc_cpuset_isfull(hwloc_const_cpuset_t set);

/** \brief Test whether set \p set1 is equal to set \p set2 */
int hwloc_cpuset_isequal (hwloc_const_cpuset_t set1, hwloc_const_cpuset_t set2);

/** \brief Test whether sets \p set1 and \p set2 intersects */
int hwloc_cpuset_intersects (hwloc_const_cpuset_t set1, hwloc_const_cpuset_t set2);

/** \brief Test whether set \p sub_set is part of set \p super_set */
int hwloc_cpuset_isincluded (hwloc_const_cpuset_t sub_set, hwloc_const_cpuset_t super_set);

/** \brief Or set \p modifier_set into set \p set */
void hwloc_cpuset_orset (hwloc_cpuset_t set, hwloc_const_cpuset_t modifier_set);

/** \brief And set \p modifier_set into set \p set */
void hwloc_cpuset_andset (hwloc_cpuset_t set, hwloc_const_cpuset_t modifier_set);

/** \brief Clear set \p modifier_set out of set \p set */
void hwloc_cpuset_clearset (hwloc_cpuset_t set, hwloc_const_cpuset_t modifier_set);

/** \brief Xor set \p set with set \p modifier_set */
void hwloc_cpuset_xorset (hwloc_cpuset_t set, hwloc_const_cpuset_t modifier_set);

/** \brief Compute the first CPU (least significant bit) in CPU set \p set */
int hwloc_cpuset_first(hwloc_const_cpuset_t set);

/** \brief Compute the last CPU (most significant bit) in CPU set \p set */
int hwloc_cpuset_last(hwloc_const_cpuset_t set);

/** \brief Keep a single CPU among those set in CPU set \p set
 *
 * Might be used before binding so that the process does not
 * have a chance of migrating between multiple logical CPUs
 * in the original mask.
 */
void hwloc_cpuset_singlify(hwloc_cpuset_t set);

/** \brief Compar CPU sets \p set1 and \p set2 using their first set bit.
 *
 * Smaller least significant bit is smaller.
 * The empty CPU set is considered higher than anything.
 */
int hwloc_cpuset_compar_first(hwloc_const_cpuset_t set1, hwloc_const_cpuset_t set2);

/** \brief Compar CPU sets \p set1 and \p set2 using their last bits.
 *
 * Higher most significant bit is higher.
 * The empty CPU set is considered lower than anything.
 */
int hwloc_cpuset_compar(hwloc_const_cpuset_t set1, hwloc_const_cpuset_t set2);

/** \brief Compute the weight of CPU set \p set */
int hwloc_cpuset_weight(hwloc_const_cpuset_t set);

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
