/*
 * Copyright © 2019-2020 Inria.  All rights reserved.
 * See COPYING in top-level directory.
 */

/** \file
 * \brief Memory node attributes.
 */

#ifndef HWLOC_MEMATTR_H
#define HWLOC_MEMATTR_H

#include "hwloc.h"

#ifdef __cplusplus
extern "C" {
#elif 0
}
#endif

/** \defgroup hwlocality_memattrs Comparing memory node attributes for finding where to allocate on
 *
 * Performance usually depends on where the memory access comes from.
 * This location is called an Initiator while the memory node is a Target.
 * The following attributes describe the performance of memory accesses
 * from an Initiator to a memory Target, for instance their latency
 * or bandwidth.
 *
 * Initiators performing these memory accesses are usually some PUs or Cores
 * (described as a CPU set).
 * In the future, this API may also provide information about other Initiators
 * given as objects, for instance GPUs.
 *
 * Hence a Core may choose where to allocate memory by comparing the
 * attributes of different memory node targets nearby.
 *
 * There are also some attributes that are system-wide.
 * Their value does not depend on a specific initiator performing
 * an access.
 * The memory node Capacity is an example of such attribute without
 * initiator.
 *
 * One way to use this API is to start with a cpuset describing the Cores where
 * a program is bound. The best target NUMA node for allocating memory in this
 * program on these Cores may be obtained by passing this cpuset as an initiator
 * to hwloc_memattr_get_best_target() with the relevant memory attribute.
 *
 * \note The interface actually also accepts targets that are not NUMA nodes.
 * @{
 */

/** \brief Memory node attributes. */
enum hwloc_memattr_id_e {
  /** \brief "Capacity".
   * The capacity is returned in bytes
   * (local_memory attribute in objects).
   *
   * Best capacity nodes are nodes with <b>higher capacity</b>.
   *
   * No initiator is involved when looking at this attribute.
   * The corresponding attribute flags are ::HWLOC_MEMATTR_FLAG_HIGHER_FIRST.
   */
  HWLOC_MEMATTR_ID_CAPACITY = 0,

  /** \brief "Locality".
   * The locality is returned as the number of PUs in that locality
   * (e.g. the weight of its cpuset).
   *
   * Best locality nodes are nodes with <b>smaller locality</b>
   * (nodes that are local to very few PUs).
   * Poor locality nodes are nodes with larger locality
   * (nodes that are local to the entire machine).
   *
   * No initiator is involved when looking at this attribute.
   * The corresponding attribute flags are ::HWLOC_MEMATTR_FLAG_HIGHER_FIRST.
   */
  HWLOC_MEMATTR_ID_LOCALITY = 1,

  /** \brief "Bandwidth".
   * The bandwidth is returned in MiB/s, as seen from the given initiator location.
   * Best bandwidth nodes are nodes with <b>higher bandwidth</b>.
   * The corresponding attribute flags are ::HWLOC_MEMATTR_FLAG_HIGHER_FIRST
   * and ::HWLOC_MEMATTR_FLAG_NEED_INITIATOR.
   */
  HWLOC_MEMATTR_ID_BANDWIDTH = 2,

  /** \brief "Latency".
   * The latency is returned as nanoseconds, as seen from the given initiator location.
   * Best latency nodes are nodes with <b>smaller latency</b>.
   * The corresponding attribute flags are ::HWLOC_MEMATTR_FLAG_LOWER_FIRST
   * and ::HWLOC_MEMATTR_FLAG_NEED_INITIATOR.
   */
  HWLOC_MEMATTR_ID_LATENCY = 3

  /* TODO read vs write, persistence? */
};

/** \brief A memory attribute identifier.
 * May be either one of ::hwloc_memattr_id_e or a new id returned by hwloc_memattr_register().
 */
typedef unsigned hwloc_memattr_id_t;

/** \brief Return the identifier of the memory attribute with the given name.
 */
HWLOC_DECLSPEC int
hwloc_memattr_get_by_name(hwloc_topology_t topology,
                          const char *name,
                          hwloc_memattr_id_t *id);


/** \brief Where to measure attributes from. */
struct hwloc_location {
  /** \brief Type of location. */
  enum hwloc_location_type_e {
    /** \brief Location is given as an object, in the location object union field. */
    HWLOC_LOCATION_TYPE_OBJECT = 0,
    /** \brief Location is given as an cpuset, in the location cpuset union field. */
    HWLOC_LOCATION_TYPE_CPUSET = 1
  } type;
  /** \brief Actual location. */
  union hwloc_location_u {
    /** \brief Location as an object, when the location type is HWLOC_LOCATION_TYPE_OBJECT. */
    hwloc_obj_t object;
    /** \brief Location as a cpuset, when the location type is HWLOC_LOCATION_TYPE_CPUSET. */
    hwloc_cpuset_t cpuset;
  } location;
};



/** \brief Return an attribute value for a specific target NUMA node.
 *
 * If the attribute does not relate to a specific initiator
 * (it does not have the flag ::HWLOC_MEMATTR_FLAG_NEED_INITIATOR),
 * location \p initiator is ignored and may be \c NULL.
 *
 * \p flags must be \c 0 for now.
 */
HWLOC_DECLSPEC int
hwloc_memattr_get_value(hwloc_topology_t topology,
                        hwloc_memattr_id_t attribute,
                        hwloc_obj_t target_node,
                        struct hwloc_location *initiator,
                        unsigned long flags,
                        hwloc_uint64_t *value);

/** \brief Return the best target NUMA node for the given attribute and initiator.
 *
 * If the attribute does not relate to a specific initiator
 * (it does not have the flag ::HWLOC_MEMATTR_FLAG_NEED_INITIATOR),
 * location \p initiator is ignored and may be \c NULL.
 *
 * If \p value is non \c NULL, the corresponding value is returned there.
 *
 * If multiple targets have the same attribute values, only one is
 * returned (and there is no way to clarify how that one is chosen).
 * Applications that want to detect targets with identical/similar
 * values, or that want to look at values for multiple attributes,
 * should rather get all values using hwloc_memattr_get_value()
 * and manually select the target they consider the best.
 *
 * \p flags must be \c 0 for now.
 *
 * If there are no matching targets, \c -1 is returned with \p errno set to \c ENOENT;
 */
HWLOC_DECLSPEC int
hwloc_memattr_get_best_target(hwloc_topology_t topology,
                              hwloc_memattr_id_t attribute,
                              struct hwloc_location *initiator,
                              unsigned long flags,
                              hwloc_obj_t *best_target, hwloc_uint64_t *value);

/** \brief Return the best initiator for the given attribute and target NUMA node.
 *
 * If the attribute does not relate to a specific initiator
 * (it does not have the flag ::HWLOC_MEMATTR_FLAG_NEED_INITIATOR),
 * \c -1 is returned and \p errno is set to \c EINVAL.
 *
 * If \p value is non \c NULL, the corresponding value is returned there.
 *
 * If multiple initiators have the same attribute values, only one is
 * returned (and there is no way to clarify how that one is chosen).
 * Applications that want to detect initiators with identical/similar
 * values, or that want to look at values for multiple attributes,
 * should rather get all values using hwloc_memattr_get_value()
 * and manually select the initiator they consider the best.
 *
 * The returned initiator should not be modified or freed,
 * it belongs to the topology.
 *
 * \p flags must be \c 0 for now.
 *
 * If there are no matching initiators, \c -1 is returned with \p errno set to \c ENOENT;
 */
HWLOC_DECLSPEC int
hwloc_memattr_get_best_initiator(hwloc_topology_t topology,
                                 hwloc_memattr_id_t attribute,
                                 hwloc_obj_t target,
                                 unsigned long flags,
                                 struct hwloc_location *best_initiator, hwloc_uint64_t *value);

/** @} */


/** \defgroup hwlocality_memattrs_manage Managing memory attributes
 * @{
 */

/** \brief Return the name of a memory attribute.
 */
HWLOC_DECLSPEC int
hwloc_memattr_get_name(hwloc_topology_t topology,
                       hwloc_memattr_id_t attribute,
                       const char **name);

/** \brief Return the flags of the given attribute.
 *
 * Flags are a OR'ed set of ::hwloc_memattr_flag_e.
 */
HWLOC_DECLSPEC int
hwloc_memattr_get_flags(hwloc_topology_t topology,
                        hwloc_memattr_id_t attribute,
                        unsigned long *flags);

/** \brief Memory attribute flags.
 * Given to hwloc_memattr_register() and returned by hwloc_memattr_get_flags().
 */
enum hwloc_memattr_flag_e {
  /** \brief The best nodes for this memory attribute are those with the higher values.
   * For instance Bandwidth.
   */
  HWLOC_MEMATTR_FLAG_HIGHER_FIRST = (1UL<<0),
  /** \brief The best nodes for this memory attribute are those with the lower values.
   * For instance Latency.
   */
  HWLOC_MEMATTR_FLAG_LOWER_FIRST = (1UL<<1),
  /** \brief The value returned for this memory attribute depends on the given initiator.
   * For instance Bandwidth and Latency, but not Capacity.
   */
  HWLOC_MEMATTR_FLAG_NEED_INITIATOR = (1UL<<2)
};

/** \brief Register a new memory attribute.
 *
 * Add a specific memory attribute that is not defined in ::hwloc_memattr_id_e.
 * Flags are a OR'ed set of ::hwloc_memattr_flag_e. It must contain at least
 * one of ::HWLOC_MEMATTR_FLAG_HIGHER_FIRST or ::HWLOC_MEMATTR_FLAG_LOWER_FIRST.
 */
HWLOC_DECLSPEC int
hwloc_memattr_register(hwloc_topology_t topology,
                       const char *name,
                       unsigned long flags,
                       hwloc_memattr_id_t *id);

/** \brief Set an attribute value for a specific target NUMA node.
 *
 * If the attribute does not relate to a specific initiator
 * (it does not have the flag ::HWLOC_MEMATTR_FLAG_NEED_INITIATOR),
 * location \p initiator is ignored and may be \c NULL.
 *
 * The initiator will be copied into the topology,
 * the caller should free anything allocated to store the initiator,
 * for instance the cpuset.
 *
 * \p flags must be \c 0 for now.
 */
HWLOC_DECLSPEC int
hwloc_memattr_set_value(hwloc_topology_t topology,
                        hwloc_memattr_id_t attribute,
                        hwloc_obj_t target_node,
                        struct hwloc_location *initiator,
                        unsigned long flags,
                        hwloc_uint64_t value);

/** \brief Return the target NUMA nodes that have some values for a given attribute.
 *
 * Return targets for the given attribute in the \p targets array
 * (for the given initiator if any).
 * If \p values is not \c NULL, the corresponding attribute values
 * are stored in the array it points to.
 *
 * On input, \p nr points to the number of targets that may be stored
 * in the array \p targets (and \p values).
 * On output, \p nr points to the number of targets (and values) that
 * were actually found, even if some of them couldn't be stored in the array.
 * Targets that couldn't be stored are ignored, but the function still
 * returns success (\c 0). The caller may find out by comparing the value pointed
 * by \p nr before and after the function call.
 *
 * The returned targets should not be modified or freed,
 * they belong to the topology.
 *
 * Argument \p initiator is ignored if the attribute does not relate to a specific
 * initiator (it does not have the flag ::HWLOC_MEMATTR_FLAG_NEED_INITIATOR).
 * Otherwise \p initiator may be non \c NULL to report only targets
 * that have a value for that initiator.
 *
 * \p flags must be \c 0 for now.
 */
HWLOC_DECLSPEC int
hwloc_memattr_get_targets(hwloc_topology_t topology,
                          hwloc_memattr_id_t attribute,
                          struct hwloc_location *initiator,
                          unsigned long flags,
                          unsigned *nrp, hwloc_obj_t *targets, hwloc_uint64_t *values);

/** \brief Return the initiators that have values for a given attribute for a specific target NUMA node.
 *
 * Return initiators for the given attribute and target node in the
 * \p initiators array.
 * If \p values is not \c NULL, the corresponding attribute values
 * are stored in the array it points to.
 *
 * On input, \p nr points to the number of initiators that may be stored
 * in the array \p initiators (and \p values).
 * On output, \p nr points to the number of initiators (and values) that
 * were actually found, even if some of them couldn't be stored in the array.
 * Initiators that couldn't be stored are ignored, but the function still
 * returns success (\c 0). The caller may find out by comparing the value pointed
 * by \p nr before and after the function call.
 *
 * The returned initiators should not be modified or freed,
 * they belong to the topology.
 *
 * \p flags must be \c 0 for now.
 *
 * If the attribute does not relate to a specific initiator
 * (it does not have the flag ::HWLOC_MEMATTR_FLAG_NEED_INITIATOR),
 * no initiator is returned.
 */
HWLOC_DECLSPEC int
hwloc_memattr_get_initiators(hwloc_topology_t topology,
                             hwloc_memattr_id_t attribute,
                             hwloc_obj_t target_node,
                             unsigned long flags,
                             unsigned *nr, struct hwloc_location *initiators, hwloc_uint64_t *values);
/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* HWLOC_MEMATTR_H */
