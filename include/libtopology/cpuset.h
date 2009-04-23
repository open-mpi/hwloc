/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* The cpuset API, for use in libtopology itself.  */

#ifndef LIBTOPOLOGY_CPUSET_H
#define LIBTOPOLOGY_CPUSET_H

#include <stdint.h>
#include <strings.h>
#include <libtopology/configuration.h>


#if LIBTOPO_NBMAXCPUS <= 32
typedef uint32_t lt_cpuset_t; /* FIXME: uint_fast32_t if available? */
/** \brief Set with no or all cpus selected. */
#    define LT_CPUSET_ZERO		((lt_cpuset_t) 0U)
#    define LT_CPUSET_FULL		((lt_cpuset_t) ~0U)
/** \brief Set with only \e cpu in selected. */
#    define LT_CPUSET_CPU(cpu)		((lt_cpuset_t)(1U<<(cpu)))
/** \brief Format string snippet suitable for the cpuset datatype */
#    define LT_PRIxCPUSET			"08x"
#    define LT_CPUSET_PRINTF_VALUE(x)	((unsigned)(x))

#elif LIBTOPO_NBMAXCPUS <= 64
typedef uint64_t lt_cpuset_t;
#    define LT_CPUSET_ZERO		((lt_cpuset_t) 0ULL)
#    define LT_CPUSET_FULL		((lt_cpuset_t) ~0ULL)
#    define LT_CPUSET_CPU(cpu)		((lt_cpuset_t)(1ULL<<(cpu)))
#    define LT_PRIxCPUSET			"016llx"
#    define LT_CPUSET_PRINTF_VALUE(x)	((unsigned long long)(x))

#else /* large cpuset using an array of unsigned long subsets */
#    define LT_HAVE_CPUSUBSET	1 /* marker for CPUSUBSET being enabled */
/* size and count of subsets within a set */
#    define LT_CPUSUBSET_SIZE		(8*sizeof(long))
#    define LT_CPUSUBSET_COUNT		((CPU_NBMAXCPUS+LT_CPUSUBSET_SIZE-1)/LT_CPUSUBSET_SIZE)
typedef struct { unsigned long s[LT_CPUSUBSET_COUNT]; } lt_cpuset_t;
/* extract a subset from a set using an index or a cpu */
#    define LT_CPUSUBSET_SUBSET(set,x)	((set).s[x])
#    define LT_CPUSUBSET_INDEX(cpu)	((cpu)/(LT_CPUSUBSET_SIZE))
#    define LT_CPUSUBSET_CPUSUBSET(set,cpu)	LT_CPUSUBSET_SUBSET(set,LT_CPUSUBSET_INDEX(cpu))
/* predefined subset values */
#    define LT_CPUSUBSET_VAL(cpu)	(1UL<<((cpu)%(LT_CPUSUBSET_SIZE)))
#    define LT_CPUSUBSET_ZERO		0UL
#    define LT_CPUSUBSET_FULL		~0UL
/* actual whole-cpuset values */
#    define LT_CPUSET_ZERO		(lt_cpuset_t){ .s[0 ... LT_CPUSUBSET_COUNT-1] = LT_CPUSUBSET_ZERO }
#    define LT_CPUSET_FULL		(lt_cpuset_t){ .s[0 ... LT_CPUSUBSET_COUNT-1] = LT_CPUSUBSET_FULL }
#    define LT_CPUSET_CPU(cpu)		({ lt_cpuset_t __set = LT_CPUSET_ZERO; LT_CPUSUBSET_CPUSUBSET(__set,cpu) = LT_CPUSUBSET_VAL(cpu); __set; })
/* displaying cpusets */
#if LT_BITS_PER_LONG == 32
#    define LT_PRIxCPUSUBSET		"%08lx"
#else
#    define LT_PRIxCPUSUBSET		"%016lx"
#endif
#    define LT_CPUSUBSET_STRING_LENGTH	(LT_BITS_PER_LONG/4)
#    define LT_CPUSET_STRING_LENGTH	(LT_CPUSUBSET_COUNT*(LT_CPUSUBSET_STRING_LENGTH+1))
#    define LT_PRIxCPUSET		"s"
#    define LT_CPUSET_PRINTF_VALUE(x)	({			\
	char *__buf = alloca(LT_CPUSET_STRING_LENGTH+1);		\
	char *__tmp = __buf;						\
	int __i;							\
	for(__i=LT_CPUSUBSET_COUNT-1; __i>=0; __i--)			\
	  __tmp += sprintf(__tmp, LT_PRIxCPUSUBSET ",", (x).s[__i]);	\
	*(__tmp-1) = '\0';						\
	__buf;								\
     })
#endif /* large cpuset using an array of unsigned long subsets */


#ifndef LT_HAVE_CPUSUBSET
#   define LT_CPUSUBSET_SIZE 		0 /* useless */
#   define LT_CPUSUBSET_COUNT		1
#   define LT_CPUSUBSET_SUBSET(set,x)	(set)
#   define LT_CPUSUBSET_CPUSUBSET(set,cpu)	(set)
#   define LT_CPUSUBSET_VAL(cpu)	LT_CPUSET_CPU(cpu)
#   define LT_CPUSUBSET_ZERO		LT_CPUSET_ZERO
#   define LT_CPUSUBSET_FULL		LT_CPUSET_FULL
#endif



/*  Primitives & macros for building "sets" of cpus. */

/** \brief Initialize CPU set */
void lt_cpuset_init(lt_cpuset_t * set);
#define lt_cpuset_init(m)        lt_cpuset_zero(m)

/** \brief Empty CPU set */
static __inline__ void lt_cpuset_zero(lt_cpuset_t * set);
static __inline__ void lt_cpuset_zero(lt_cpuset_t * set)
{
	int i;
	for(i=0; i<LT_CPUSUBSET_COUNT; i++)
		LT_CPUSUBSET_SUBSET(*set,i) = LT_CPUSUBSET_ZERO;
}

/** \brief Fill CPU set */
static __inline__ void lt_cpuset_fill(lt_cpuset_t * set);
static __inline__ void lt_cpuset_fill(lt_cpuset_t * set)
{
	int i;
	for(i=0; i<LT_CPUSUBSET_COUNT; i++)
		LT_CPUSUBSET_SUBSET(*set,i) = LT_CPUSUBSET_FULL;
}

/** \brief Clear CPU set and set CPU \e cpu */
static __inline__ void lt_cpuset_cpu(lt_cpuset_t * set,
    unsigned cpu);
static __inline__ void lt_cpuset_cpu(lt_cpuset_t * set,
    unsigned cpu)
{
#ifndef LT_HAVE_CPUSUBSET
	*set = LT_CPUSET_CPU(cpu);
#else
	lt_cpuset_zero(set);
	LT_CPUSUBSET_CPUSUBSET(*set,cpu) |= LT_CPUSUBSET_VAL(cpu);
#endif
}

/** \brief Clear CPU set and set all but the CPU \e cpu */
static __inline__ void lt_cpuset_all_but_cpu(lt_cpuset_t * set,
    unsigned cpu);
static __inline__ void lt_cpuset_all_but_cpu(lt_cpuset_t * set,
    unsigned cpu)
{
#ifndef LT_HAVE_CPUSUBSET
	*set = ~LT_CPUSET_CPU(cpu);
#else
	lt_cpuset_fill(set);
	LT_CPUSUBSET_CPUSUBSET(*set,cpu) &= ~LT_CPUSUBSET_VAL(cpu);
#endif
}

/** \brief Add CPU \e cpu in CPU set \e set */
static __inline__ void lt_cpuset_set(lt_cpuset_t * set,
    unsigned cpu);
static __inline__ void lt_cpuset_set(lt_cpuset_t * set,
    unsigned cpu)
{
#ifndef LT_HAVE_CPUSUBSET
	*set |= LT_CPUSET_CPU(cpu);
#else
	LT_CPUSUBSET_CPUSUBSET(*set,cpu) |= LT_CPUSUBSET_VAL(cpu);
#endif
}

/** \brief Remove CPU \e cpu from CPU set \e set */
static __inline__ void lt_cpuset_clr(lt_cpuset_t * set,
    unsigned cpu);
static __inline__ void lt_cpuset_clr(lt_cpuset_t * set,
    unsigned cpu)
{
#ifndef LT_HAVE_CPUSUBSET
	*set &= ~(LT_CPUSET_CPU(cpu));
#else
	LT_CPUSUBSET_CPUSUBSET(*set,cpu) &= ~LT_CPUSUBSET_VAL(cpu);
#endif
}

/** \brief Test whether CPU \e cpu is part of set \e set */
static __inline__ int lt_cpuset_isset(const lt_cpuset_t * set,
    unsigned cpu);
static __inline__ int lt_cpuset_isset(const lt_cpuset_t * set,
    unsigned cpu)
{
#ifndef LT_HAVE_CPUSUBSET
	return 1 & (*set >> cpu);
#else
	return (LT_CPUSUBSET_CPUSUBSET(*set,cpu) & LT_CPUSUBSET_VAL(cpu)) != 0;
#endif
}

/** \brief Test whether set \e set is zero or full */
static __inline__ int lt_cpuset_iszero(const lt_cpuset_t *set);
static __inline__ int lt_cpuset_isfull(const lt_cpuset_t *set);
static __inline__ int lt_cpuset_iszero(const lt_cpuset_t *set)
{
	int i;
	for(i=0; i<LT_CPUSUBSET_COUNT; i++)
		if (LT_CPUSUBSET_SUBSET(*set,i) != LT_CPUSUBSET_ZERO)
			return 0;
	return 1;
}

static __inline__ int lt_cpuset_isfull(const lt_cpuset_t *set)
{
	int i;
	for(i=0; i<LT_CPUSUBSET_COUNT; i++)
		if (LT_CPUSUBSET_SUBSET(*set,i) != LT_CPUSUBSET_FULL)
			return 0;
	return 1;
}

/** \brief Test whether set \e set1 is equal to set \e set2 */
static __inline__ int lt_cpuset_isequal (const lt_cpuset_t *set1,
					 const lt_cpuset_t *set2);
static __inline__ int lt_cpuset_isequal (const lt_cpuset_t *set1,
					 const lt_cpuset_t *set2)
{
	int i;
	for(i=0; i<LT_CPUSUBSET_COUNT; i++)
		if (LT_CPUSUBSET_SUBSET(*set1,i) != LT_CPUSUBSET_SUBSET(*set2,i))
			return 0;
	return 1;
}

/** \brief Test whether set \e sub_set is part of set \e super_set */
static __inline__ int lt_cpuset_isincluded (const lt_cpuset_t *super_set,
					    const lt_cpuset_t *sub_set);
static __inline__ int lt_cpuset_isincluded (const lt_cpuset_t *super_set,
					    const lt_cpuset_t *sub_set)
{
	int i;
	for(i=0; i<LT_CPUSUBSET_COUNT; i++)
		if (LT_CPUSUBSET_SUBSET(*super_set,i) != (LT_CPUSUBSET_SUBSET(*super_set,i) | LT_CPUSUBSET_SUBSET(*sub_set,i)))
			return 0;
	return 1;
}

/** \brief Or set \e modifier_set into set \e set */
static __inline__ void lt_cpuset_orset (lt_cpuset_t *set,
					const lt_cpuset_t *modifier_set);
static __inline__ void lt_cpuset_orset (lt_cpuset_t *set,
					const lt_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<LT_CPUSUBSET_COUNT; i++)
		LT_CPUSUBSET_SUBSET(*set,i) |= LT_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief And set \e modifier_set into set \e set */
static __inline__ void lt_cpuset_andset (lt_cpuset_t *set,
					 const lt_cpuset_t *modifier_set);
static __inline__ void lt_cpuset_andset (lt_cpuset_t *set,
					 const lt_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<LT_CPUSUBSET_COUNT; i++)
		LT_CPUSUBSET_SUBSET(*set,i) &= LT_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief Clear set \e modifier_set out of set \e set */
static __inline__ void lt_cpuset_clearset (lt_cpuset_t *set,
					   const lt_cpuset_t *modifier_set);
static __inline__ void lt_cpuset_clearset (lt_cpuset_t *set,
					   const lt_cpuset_t *modifier_set)
{
	int i;
	for(i=0; i<LT_CPUSUBSET_COUNT; i++)
		LT_CPUSUBSET_SUBSET(*set,i) &= ~LT_CPUSUBSET_SUBSET(*modifier_set,i);
}

/** \brief Compute the first CPU in CPU mask */
static __inline__ int lt_cpuset_first(const lt_cpuset_t * cpuset);
static __inline__ int lt_cpuset_first(const lt_cpuset_t * cpuset)
{
#if (!defined LT_HAVE_CPUSUBSET) && LT_BITS_PER_LONG < LIBTOPO_NBMAXCPUS
	/* FIXME: return ma_ffs64(*cpuset)-1 */
	int i;
	i = ffs(((unsigned*)cpuset)[0]);
	if (i>0)
		return i-1;
	i = ffs(((unsigned*)cpuset)[1]);
	if (i>0)
		return i+32-1;
	return -1;
#else
	int i;
	for(i=0; i<LT_CPUSUBSET_COUNT; i++) {
		int _ffs = ffs(LT_CPUSUBSET_SUBSET(*cpuset,i));
		if (_ffs>0)
			return _ffs - 1 + LT_CPUSUBSET_SIZE*i;
	}

	return -1;
#endif
}

/** \brief Loop macro iterating on a cpuset and yielding on each cpu that
 *  is member of the set.
 *  Uses variables \e set (the cpu set) and \e cpu (the loop variable) */
#define lt_cpuset_foreach_begin(cpu, cpuset) \
        for (cpu = 0; cpu < LT_NBMAXCPUS; cpu++) \
                if (lt_cpuset_isset(cpuset, cpu)) {
#define marcel_cpuset_foreach_end() \
                }

#endif /* LIBTOPOLOGY_CPUSET_H */
