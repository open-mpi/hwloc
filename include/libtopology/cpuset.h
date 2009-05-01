/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* The cpuset API, for use in libtopology itself.  */

#ifndef LIBTOPOLOGY_CPUSET_H
#define LIBTOPOLOGY_CPUSET_H

#include <libtopology/config.h>

#include <stdint.h>
#include <string.h>
#include <strings.h>

/* large cpuset using an array of unsigned long subsets */

/* size and count of subsets within a set */
#    define TOPO_CPUSUBSET_SIZE		(8*sizeof(long))
#    define TOPO_CPUSUBSET_COUNT	((LIBTOPO_NBMAXCPUS+TOPO_CPUSUBSET_SIZE-1)/TOPO_CPUSUBSET_SIZE)
typedef struct { unsigned long s[TOPO_CPUSUBSET_COUNT]; } topo_cpuset_t;

/* extract a subset from a set using an index or a cpu */
#    define TOPO_CPUSUBSET_SUBSET(set,x)	((set).s[x])
#    define TOPO_CPUSUBSET_INDEX(cpu)		((cpu)/(TOPO_CPUSUBSET_SIZE))
#    define TOPO_CPUSUBSET_CPUSUBSET(set,cpu)	TOPO_CPUSUBSET_SUBSET(set,TOPO_CPUSUBSET_INDEX(cpu))

/* predefined subset values */
#    define TOPO_CPUSUBSET_VAL(cpu)	(1UL<<((cpu)%(TOPO_CPUSUBSET_SIZE)))
#    define TOPO_CPUSUBSET_ZERO		0UL
#    define TOPO_CPUSUBSET_FULL		~0UL

/* actual whole-cpuset values */
#    define TOPO_CPUSET_ZERO		(topo_cpuset_t){ .s[0 ... TOPO_CPUSUBSET_COUNT-1] = TOPO_CPUSUBSET_ZERO }
#    define TOPO_CPUSET_FULL		(topo_cpuset_t){ .s[0 ... TOPO_CPUSUBSET_COUNT-1] = TOPO_CPUSUBSET_FULL }
#    define TOPO_CPUSET_CPU(cpu)	({ topo_cpuset_t __set = TOPO_CPUSET_ZERO; TOPO_CPUSUBSET_CPUSUBSET(__set,cpu) = TOPO_CPUSUBSET_VAL(cpu); __set; })

/* displaying cpusets */
#if TOPO_BITS_PER_LONG == 32
#    define TOPO_PRIxCPUSUBSET		"%08lx"
#else
#    define TOPO_PRIxCPUSUBSET		"%016lx"
#endif
#    define TOPO_CPUSUBSET_STRING_LENGTH	(TOPO_BITS_PER_LONG/4)
#    define TOPO_CPUSET_STRING_LENGTH		(TOPO_CPUSUBSET_COUNT*(TOPO_CPUSUBSET_STRING_LENGTH+1))
#    define TOPO_PRIxCPUSET		"s"
#    define TOPO_CPUSET_PRINTF_VALUE(x)	({				\
	char *__buf = alloca(TOPO_CPUSET_STRING_LENGTH+1);		\
	char *__tmp = __buf;						\
	int __i;							\
	for(__i=TOPO_CPUSUBSET_COUNT-1; __i>=0; __i--)			\
	  __tmp += sprintf(__tmp, TOPO_PRIxCPUSUBSET ",", (x).s[__i]);	\
	*(__tmp-1) = '\0';						\
	__buf;								\
     })



/*  Primitives & macros for building "sets" of cpus. */

/** \brief Initialize CPU set */
void topo_cpuset_init(topo_cpuset_t * set);
#define topo_cpuset_init(m)        topo_cpuset_zero(m)

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

/** \brief Test whether set \e sub_set is part of set \e super_set */
static __inline__ int topo_cpuset_isincluded (const topo_cpuset_t *super_set,
					      const topo_cpuset_t *sub_set);
static __inline__ int topo_cpuset_isincluded (const topo_cpuset_t *super_set,
					      const topo_cpuset_t *sub_set)
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

static __inline__ int topo_ffsl(unsigned long x);
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
static __inline__ int topo_ffs32(unsigned long x);
static __inline__ int topo_ffs32(unsigned long x)
{
	return topo_ffs(x
#if TOPO_BITS_PER_INT == 16
			& 0xfffful) ? : (topo_ffs(x >> 16) + 16
#endif
		);
}

/* Then make it 64 bit if longs are.  */
static __inline__ int topo_ffsl(unsigned long x);
static __inline__ int topo_ffsl(unsigned long x)
{
	return topo_ffs32(x
#if TOPO_BITS_PER_LONG == 64
			& 0xfffffffful) ? : (topo_ffs(x >> 32) + 32
#endif
		);
}
#endif

/** \brief Compute the first CPU in CPU mask */
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

#endif /* LIBTOPOLOGY_CPUSET_H */
