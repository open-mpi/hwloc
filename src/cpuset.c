/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <private/misc.h>
#include <private/private.h>
#include <hwloc/cpuset.h>

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>

/* TODO
 *
 * - convert infinite-flag into 0xf...f, prefixing strings
 * - document infinite cpusets, that last and weight return -1
 *   and that the foreach loop will never end
 * - drop NBMAXCPUS entirely in other files
 *
 * - optimize some stuff (see TODO below)
 * - add realloc_mayreduce to reduce ulongs_count when possible
 *   without deallocating, so that the number of ulongs to manage
 *   is smaller, but reallocating larger is trivial
 */

/* magic number */
#define HWLOC_CPUSET_MAGIC 0x20091007

/* actual opaque type internals */
struct hwloc_cpuset_s {
  unsigned ulongs_count;
  /* TODO add ulongs_allocated_counts so that we may reduce the used size without reallocating to smaller */
  unsigned long *ulongs;
  int infinite; /* set to 1 if all bits beyond ulongs are set */
#ifdef HWLOC_DEBUG
  int magic;
#endif
};

/* overzealous check in debug-mode, not as powerful as valgrind but still useful */
#ifdef HWLOC_DEBUG
#define HWLOC__CPUSET_CHECK(set) assert((set)->magic == HWLOC_CPUSET_MAGIC)
#else
#define HWLOC__CPUSET_CHECK(set)
#endif

/* extract a subset from a set using an index or a cpu */
#define HWLOC_CPUSUBSET_SUBSET(set,x)		((set).ulongs[x])
#define HWLOC_CPUSUBSET_INDEX(cpu)		((cpu)/(HWLOC_BITS_PER_LONG))
#define HWLOC_CPUSUBSET_CPUSUBSET(set,cpu)	HWLOC_CPUSUBSET_SUBSET(set,HWLOC_CPUSUBSET_INDEX(cpu))

/* predefined subset values */
#define HWLOC_CPUSUBSET_VAL(cpu)		(1UL<<((cpu)%(HWLOC_BITS_PER_LONG)))
#define HWLOC_CPUSUBSET_ZERO			0UL
#define HWLOC_CPUSUBSET_FULL			~0UL


struct hwloc_cpuset_s * hwloc_cpuset_alloc(void)
{
  struct hwloc_cpuset_s * set;
  set = malloc(sizeof(struct hwloc_cpuset_s));
  if (!set)
    return NULL;

  set->ulongs_count = 1;
  set->ulongs = calloc(sizeof(unsigned long), set->ulongs_count);
  if (!set->ulongs) {
    free(set->ulongs);
    return NULL;
  }

  set->infinite = 0;
#ifdef HWLOC_DEBUG
  set->magic = HWLOC_CPUSET_MAGIC;
#endif
  return set;
}

void hwloc_cpuset_free(struct hwloc_cpuset_s * set)
{
  if (!set)
    return;

  HWLOC__CPUSET_CHECK(set);
#ifdef HWLOC_DEBUG
  set->magic = 0;
#endif

  free(set->ulongs);
  free(set);
}

/* realloc until it contains at least needed_count ulongs */
static void
hwloc_cpuset_realloc_by_ulongs(struct hwloc_cpuset_s * set, unsigned needed_count)
{
  unsigned ulongs_count = set->ulongs_count;
  unsigned i;

  HWLOC__CPUSET_CHECK(set);

  if (needed_count <= ulongs_count)
    return;

  while (ulongs_count < needed_count)
    ulongs_count *= 2;

  set->ulongs = realloc(set->ulongs, ulongs_count * sizeof(unsigned long));
  assert(set->ulongs);

  /* fill the newly allocated subset depending on the infinite flag */
  for(i=set->ulongs_count; i<ulongs_count; i++)
    set->ulongs[i] = set->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;

  set->ulongs_count = ulongs_count;
}

/* realloc until it contains at least cpu+1 bits */
#define hwloc_cpuset_realloc_by_cpu_index(set, cpu) hwloc_cpuset_realloc_by_ulongs(set, ((cpu)/HWLOC_BITS_PER_LONG)+1)

struct hwloc_cpuset_s * hwloc_cpuset_dup(const struct hwloc_cpuset_s * old)
{
  struct hwloc_cpuset_s * new;

  HWLOC__CPUSET_CHECK(old);

  new = malloc(sizeof(struct hwloc_cpuset_s));
  if (!new)
    return NULL;

  memcpy(new, old, sizeof(struct hwloc_cpuset_s));

  new->ulongs = malloc(old->ulongs_count * sizeof(unsigned long));
  if (!new->ulongs) {
    free(new);
    return NULL;
  }

  memcpy(new->ulongs, old->ulongs, new->ulongs_count * sizeof(unsigned long));
  return new;
}

void hwloc_cpuset_copy(struct hwloc_cpuset_s * dst, const struct hwloc_cpuset_s * src)
{
  int i;

  HWLOC__CPUSET_CHECK(dst);
  HWLOC__CPUSET_CHECK(src);

  hwloc_cpuset_realloc_by_ulongs(dst, src->ulongs_count);

  dst->infinite = src->infinite;
  memcpy(dst->ulongs, src->ulongs, src->ulongs_count * sizeof(unsigned long));
  for(i=src->ulongs_count; i<dst->ulongs_count; i++)
    dst->ulongs[i] = dst->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
}

static unsigned hwloc_cpuset_maxnbcpus(const struct hwloc_cpuset_s * set)
{
  return set->ulongs_count * HWLOC_BITS_PER_LONG;
}

/* Strings always use 32bit groups */
#define HWLOC_PRIxCPUSUBSET		"%08lx"
#define HWLOC_CPUSET_SUBSTRING_SIZE	32
#define HWLOC_CPUSET_SUBSTRING_COUNT	((HWLOC_NBMAXCPUS+HWLOC_CPUSET_SUBSTRING_SIZE-1)/HWLOC_CPUSET_SUBSTRING_SIZE)
#define HWLOC_CPUSET_SUBSTRING_LENGTH	(HWLOC_CPUSET_SUBSTRING_SIZE/4)

int hwloc_cpuset_snprintf(char * __hwloc_restrict buf, size_t buflen, const struct hwloc_cpuset_s * __hwloc_restrict set)
{
  ssize_t size = buflen;
  char *tmp = buf;
  int res, ret = 0;
  int needcomma = 0;
  int i;
  unsigned long accum = 0;
  int accumed = 0;
#if HWLOC_BITS_PER_LONG == HWLOC_CPUSET_SUBSTRING_SIZE
  const unsigned long accum_mask = ~0UL;
#else /* HWLOC_BITS_PER_LONG != HWLOC_CPUSET_SUBSTRING_SIZE */
  const unsigned long accum_mask = ((1UL << HWLOC_CPUSET_SUBSTRING_SIZE) - 1) << (HWLOC_BITS_PER_LONG - HWLOC_CPUSET_SUBSTRING_SIZE);
#endif /* HWLOC_BITS_PER_LONG != HWLOC_CPUSET_SUBSTRING_SIZE */

  HWLOC__CPUSET_CHECK(set);

  /* mark the end in case we do nothing later */
  if (buflen > 0)
    tmp[0] = '\0';

  i=set->ulongs_count-1;
  while (i>=0 || accumed) {
    /* Refill accumulator */
    if (!accumed) {
      accum = set->ulongs[i--];
      accumed = HWLOC_BITS_PER_LONG;
    }

    if (accum & accum_mask) {
      /* print the whole subset if not empty */
        res = hwloc_snprintf(tmp, size, needcomma ? ",0x" HWLOC_PRIxCPUSUBSET : "0x" HWLOC_PRIxCPUSUBSET,
		     (accum & accum_mask) >> (HWLOC_BITS_PER_LONG - HWLOC_CPUSET_SUBSTRING_SIZE));
      needcomma = 1;
    } else if (i == -1 && accumed == HWLOC_CPUSET_SUBSTRING_SIZE) {
      /* print a single 0 to mark the last subset */
      res = hwloc_snprintf(tmp, size, needcomma ? ",0x0" : "0x0");
    } else if (needcomma) {
      res = hwloc_snprintf(tmp, size, ",");
    } else {
      res = 0;
    }
    if (res < 0)
      return -1;
    ret += res;

#if HWLOC_BITS_PER_LONG == HWLOC_CPUSET_SUBSTRING_SIZE
    accum = 0;
    accumed = 0;
#else
    accum <<= HWLOC_CPUSET_SUBSTRING_SIZE;
    accumed -= HWLOC_CPUSET_SUBSTRING_SIZE;
#endif

    if (res >= size)
      res = size>0 ? size - 1 : 0;

    tmp += res;
    size -= res;
  }

  return ret;
}

int hwloc_cpuset_asprintf(char ** strp, const struct hwloc_cpuset_s * __hwloc_restrict set)
{
  int len;
  char *buf;

  HWLOC__CPUSET_CHECK(set);

  len = hwloc_cpuset_snprintf(NULL, 0, set);
  buf = malloc(len+1);
  *strp = buf;
  return hwloc_cpuset_snprintf(buf, len+1, set);
}

int hwloc_cpuset_from_string(struct hwloc_cpuset_s *set, const char * __hwloc_restrict string)
{
  const char * current = string;
  int count=0;

  hwloc_cpuset_zero(set);

  /* count how many substrings there are */
  count++;
  while ((current = strchr(current+1, ',')) != NULL)
    count++;

  hwloc_cpuset_realloc_by_ulongs(set, count / (HWLOC_BITS_PER_LONG/HWLOC_CPUSET_SUBSTRING_SIZE) + 1);

  current = string;
  while (*current != '\0') {
    unsigned long val;
    char *next;
    val = strtoul(current, &next, 16);

    count--;
    set->ulongs[count / (HWLOC_BITS_PER_LONG/HWLOC_CPUSET_SUBSTRING_SIZE)] |= (val << ((count * HWLOC_CPUSET_SUBSTRING_SIZE) % HWLOC_BITS_PER_LONG));

    if (*next != ',')
      break;
    current = (const char*) next+1;
  }

  return 0;
}

void hwloc_cpuset_zero(struct hwloc_cpuset_s * set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	for(i=0; i<set->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_ZERO;
	set->infinite = 0;
}

void hwloc_cpuset_fill(struct hwloc_cpuset_s * set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	for(i=0; i<set->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_FULL;
	set->infinite = 1;
}

void hwloc_cpuset_from_ulong(struct hwloc_cpuset_s *set, unsigned long mask)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	HWLOC_CPUSUBSET_SUBSET(*set,0) = mask;
	for(i=1; i<set->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_ZERO;
}

void hwloc_cpuset_from_ith_ulong(struct hwloc_cpuset_s *set, int i, unsigned long mask)
{
	int j;

	HWLOC__CPUSET_CHECK(set);

	hwloc_cpuset_realloc_by_ulongs(set, i+1);

	HWLOC_CPUSUBSET_SUBSET(*set,i) = mask;
	for(j=1; j<set->ulongs_count; j++)
		if (j != i)
			HWLOC_CPUSUBSET_SUBSET(*set,j) = HWLOC_CPUSUBSET_ZERO;
}

unsigned long hwloc_cpuset_to_ulong(const struct hwloc_cpuset_s *set)
{
	HWLOC__CPUSET_CHECK(set);

	return HWLOC_CPUSUBSET_SUBSET(*set,0);
}

unsigned long hwloc_cpuset_to_ith_ulong(const struct hwloc_cpuset_s *set, int i)
{
	HWLOC__CPUSET_CHECK(set);

	if (i >= set->ulongs_count)
		return set->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	else
		return HWLOC_CPUSUBSET_SUBSET(*set,i);
}

void hwloc_cpuset_cpu(struct hwloc_cpuset_s * set, unsigned cpu)
{
	HWLOC__CPUSET_CHECK(set);

	hwloc_cpuset_zero(set);
	hwloc_cpuset_realloc_by_cpu_index(set, cpu);
	HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) |= HWLOC_CPUSUBSET_VAL(cpu);
}

void hwloc_cpuset_all_but_cpu(struct hwloc_cpuset_s * set, unsigned cpu)
{
	HWLOC__CPUSET_CHECK(set);

	hwloc_cpuset_fill(set);
	hwloc_cpuset_realloc_by_cpu_index(set, cpu);
	HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) &= ~HWLOC_CPUSUBSET_VAL(cpu);
}

void hwloc_cpuset_set(struct hwloc_cpuset_s * set, unsigned cpu)
{
	HWLOC__CPUSET_CHECK(set);

	/* TODO: don't realloc if infinite */
	hwloc_cpuset_realloc_by_cpu_index(set, cpu);
	HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) |= HWLOC_CPUSUBSET_VAL(cpu);
}

void hwloc_cpuset_set_range(struct hwloc_cpuset_s * set, unsigned begincpu, unsigned endcpu)
{
	unsigned i;

	HWLOC__CPUSET_CHECK(set);

	/* TODO: don't realloc if infinite */
	if (endcpu >= begincpu)
		hwloc_cpuset_realloc_by_cpu_index(set, endcpu);
	/* TODO: optimize */
	for (i=begincpu; i<=endcpu; i++)
		HWLOC_CPUSUBSET_CPUSUBSET(*set,i) |= HWLOC_CPUSUBSET_VAL(i);
}

void hwloc_cpuset_clr(struct hwloc_cpuset_s * set, unsigned cpu)
{
	HWLOC__CPUSET_CHECK(set);

	/* TODO: don't realloc if not infinite */
	hwloc_cpuset_realloc_by_cpu_index(set, cpu);
	HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) &= ~HWLOC_CPUSUBSET_VAL(cpu);
}

void hwloc_cpuset_clr_range(struct hwloc_cpuset_s * set, unsigned begincpu, unsigned endcpu)
{
	unsigned i;

	HWLOC__CPUSET_CHECK(set);

	/* TODO: don't realloc if infinite */
	if (endcpu >= begincpu)
		hwloc_cpuset_realloc_by_cpu_index(set, endcpu);
	/* TODO: optimize */
	for (i=begincpu; i<=endcpu; i++)
		HWLOC_CPUSUBSET_CPUSUBSET(*set,i) &= ~HWLOC_CPUSUBSET_VAL(i);
}

int hwloc_cpuset_isset(const struct hwloc_cpuset_s * set, unsigned cpu)
{
	HWLOC__CPUSET_CHECK(set);

	return cpu >= hwloc_cpuset_maxnbcpus(set) ? set->infinite : (HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) & HWLOC_CPUSUBSET_VAL(cpu)) != 0;
}

int hwloc_cpuset_iszero(const struct hwloc_cpuset_s *set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	if (set->infinite)
		return 0;
	for(i=0; i<set->ulongs_count; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*set,i) != HWLOC_CPUSUBSET_ZERO)
			return 0;
	return 1;
}

int hwloc_cpuset_isfull(const struct hwloc_cpuset_s *set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	if (!set->infinite)
		return 0;
	for(i=0; i<set->ulongs_count; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*set,i) != HWLOC_CPUSUBSET_FULL)
			return 0;
	return 1;
}

int hwloc_cpuset_isequal (const struct hwloc_cpuset_s *set1, const struct hwloc_cpuset_s *set2)
{
	unsigned long val;
	int i;

	HWLOC__CPUSET_CHECK(set1);
	HWLOC__CPUSET_CHECK(set2);

	for(i=0; i<set1->ulongs_count && i<set2->ulongs_count; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*set1,i) != HWLOC_CPUSUBSET_SUBSET(*set2,i))
			return 0;

	val = set1->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(; i<set2->ulongs_count; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*set2,i) != val)
			return 0;

	val = set2->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(; i<set1->ulongs_count; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*set1,i) != val)
			return 0;

	if (set1->infinite != set2->infinite)
		return 0;

	return 1;
}

int hwloc_cpuset_intersects (const struct hwloc_cpuset_s *set1, const struct hwloc_cpuset_s *set2)
{
	unsigned long val;
	int i;

	HWLOC__CPUSET_CHECK(set1);
	HWLOC__CPUSET_CHECK(set2);

	for(i=0; i<set1->ulongs_count && i<set2->ulongs_count; i++)
		if ((HWLOC_CPUSUBSET_SUBSET(*set1,i) & HWLOC_CPUSUBSET_SUBSET(*set2,i)) != HWLOC_CPUSUBSET_ZERO)
			return 1;

	val = set1->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(; i<set2->ulongs_count; i++)
		if ((HWLOC_CPUSUBSET_SUBSET(*set2,i) & val) != HWLOC_CPUSUBSET_ZERO)
			return 0;

	val = set2->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(; i<set1->ulongs_count; i++)
		if ((HWLOC_CPUSUBSET_SUBSET(*set1,i) & val) != HWLOC_CPUSUBSET_ZERO)
			return 0;

	if (set1->infinite && set2->infinite)
		return 0;

	return 0;
}

int hwloc_cpuset_isincluded (const struct hwloc_cpuset_s *sub_set, const struct hwloc_cpuset_s *super_set)
{
	int i;

	HWLOC__CPUSET_CHECK(sub_set);
	HWLOC__CPUSET_CHECK(super_set);

	for(i=0; i<sub_set->ulongs_count && i<super_set->ulongs_count; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*super_set,i) != (HWLOC_CPUSUBSET_SUBSET(*super_set,i) | HWLOC_CPUSUBSET_SUBSET(*sub_set,i)))
			return 0;

	for(; i<sub_set->ulongs_count; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*sub_set,i) != HWLOC_CPUSUBSET_ZERO && !super_set->infinite)
			return 0;

	if (sub_set->infinite && !super_set->infinite)
		return 0;

	return 1;
}

void hwloc_cpuset_or (struct hwloc_cpuset_s *res, const struct hwloc_cpuset_s *set1, const struct hwloc_cpuset_s *set2)
{
	const struct hwloc_cpuset_s *largest = set1->ulongs_count > set2->ulongs_count ? set1 : set2;
	const struct hwloc_cpuset_s *smallest = set1->ulongs_count > set2->ulongs_count ? set2 : set1;
	unsigned long val;
	int i;

	HWLOC__CPUSET_CHECK(res);
	HWLOC__CPUSET_CHECK(set1);
	HWLOC__CPUSET_CHECK(set2);

	hwloc_cpuset_realloc_by_ulongs(res, largest->ulongs_count);

	for(i=0; i<set1->ulongs_count && i<set2->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = HWLOC_CPUSUBSET_SUBSET(*set1,i) | HWLOC_CPUSUBSET_SUBSET(*set2,i);

	val = smallest->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(; i<largest->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = val | HWLOC_CPUSUBSET_SUBSET(*largest,i);

	val |= largest->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(; i<res->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = val;

	res->infinite = set1->infinite || set2->infinite;
}

void hwloc_cpuset_and (struct hwloc_cpuset_s *res, const struct hwloc_cpuset_s *set1, const struct hwloc_cpuset_s *set2)
{
	const struct hwloc_cpuset_s *largest = set1->ulongs_count > set2->ulongs_count ? set1 : set2;
	const struct hwloc_cpuset_s *smallest = set1->ulongs_count > set2->ulongs_count ? set2 : set1;
	unsigned long val;
	int i;

	HWLOC__CPUSET_CHECK(res);
	HWLOC__CPUSET_CHECK(set1);
	HWLOC__CPUSET_CHECK(set2);

	hwloc_cpuset_realloc_by_ulongs(res, largest->ulongs_count);

	for(i=0; i<set1->ulongs_count && i<set2->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = HWLOC_CPUSUBSET_SUBSET(*set1,i) & HWLOC_CPUSUBSET_SUBSET(*set2,i);

	val = smallest->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(; i<largest->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = val & HWLOC_CPUSUBSET_SUBSET(*largest,i);

	val &= largest->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(; i<res->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = val;

	res->infinite = set1->infinite && set2->infinite;
}

void hwloc_cpuset_andnot (struct hwloc_cpuset_s *res, const struct hwloc_cpuset_s *set1, const struct hwloc_cpuset_s *set2)
{
	const struct hwloc_cpuset_s *largest = set1->ulongs_count > set2->ulongs_count ? set1 : set2;
	const struct hwloc_cpuset_s *smallest = set1->ulongs_count > set2->ulongs_count ? set2 : set1;
	unsigned long val;
	int i;

	HWLOC__CPUSET_CHECK(res);
	HWLOC__CPUSET_CHECK(set1);
	HWLOC__CPUSET_CHECK(set2);

	hwloc_cpuset_realloc_by_ulongs(res, largest->ulongs_count);

	for(i=0; i<set1->ulongs_count && i<set2->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = HWLOC_CPUSUBSET_SUBSET(*set1,i) & ~HWLOC_CPUSUBSET_SUBSET(*set2,i);

	val = (!smallest->infinite) != (smallest != set2) ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(; i<largest->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = val & HWLOC_CPUSUBSET_SUBSET(*largest,i);

	val &= (!largest->infinite) != (largest != set2) ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(; i<res->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = val;

	res->infinite = set1->infinite && !set2->infinite;
}

void hwloc_cpuset_xor (struct hwloc_cpuset_s *res, const struct hwloc_cpuset_s *set1, const struct hwloc_cpuset_s *set2)
{
	const struct hwloc_cpuset_s *largest = set1->ulongs_count > set2->ulongs_count ? set1 : set2;
	const struct hwloc_cpuset_s *smallest = set1->ulongs_count > set2->ulongs_count ? set2 : set1;
	unsigned long val;
	int i;

	HWLOC__CPUSET_CHECK(res);
	HWLOC__CPUSET_CHECK(set1);
	HWLOC__CPUSET_CHECK(set2);

	hwloc_cpuset_realloc_by_ulongs(res, largest->ulongs_count);

	for(i=0; i<set1->ulongs_count && i<set2->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = HWLOC_CPUSUBSET_SUBSET(*set1,i) ^ HWLOC_CPUSUBSET_SUBSET(*set2,i);

	val = smallest->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(; i<largest->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = val ^ HWLOC_CPUSUBSET_SUBSET(*largest,i);

	val ^= largest->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(; i<res->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = val;

	res->infinite = (!set1->infinite) != (!set2->infinite);
}

void hwloc_cpuset_not (struct hwloc_cpuset_s *res, const struct hwloc_cpuset_s *set)
{
	unsigned long val;
	int i;

	HWLOC__CPUSET_CHECK(res);
	HWLOC__CPUSET_CHECK(set);

	hwloc_cpuset_realloc_by_ulongs(res, set->ulongs_count);

	for(i=0; i<set->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = ~HWLOC_CPUSUBSET_SUBSET(*set,i);

	val = set->infinite ? HWLOC_CPUSUBSET_ZERO : HWLOC_CPUSUBSET_FULL;
	for(; i<res->ulongs_count; i++)
		HWLOC_CPUSUBSET_SUBSET(*res,i) = val;

	res->infinite = !set->infinite;
}

int hwloc_cpuset_first(const struct hwloc_cpuset_s * set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	for(i=0; i<set->ulongs_count; i++) {
		/* subsets are unsigned longs, use ffsl */
		unsigned long w = HWLOC_CPUSUBSET_SUBSET(*set,i);
		if (w)
			return hwloc_ffsl(w) - 1 + HWLOC_BITS_PER_LONG*i;
	}

	if (set->infinite)
		return set->ulongs_count * HWLOC_BITS_PER_LONG;

	return -1;
}

int hwloc_cpuset_last(const struct hwloc_cpuset_s * set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	if (set->infinite)
		/* TODO: document */
		return -1;

	for(i=set->ulongs_count-1; i>=0; i--) {
		/* subsets are unsigned longs, use flsl */
		unsigned long w = HWLOC_CPUSUBSET_SUBSET(*set,i);
		if (w)
			return hwloc_flsl(w) - 1 + HWLOC_BITS_PER_LONG*i;
	}

	return -1;
}

int hwloc_cpuset_next(const struct hwloc_cpuset_s * set, unsigned prev_cpu)
{
	unsigned i = HWLOC_CPUSUBSET_INDEX(prev_cpu + 1);

	HWLOC__CPUSET_CHECK(set);

	if (i >= set->ulongs_count) {
		if (set->infinite)
			return prev_cpu + 1;
		else
			return -1;
	}

	for(; i<set->ulongs_count; i++) {
		/* subsets are unsigned longs, use ffsl */
		unsigned long w = HWLOC_CPUSUBSET_SUBSET(*set,i);

		/* if the prev cpu is in the same word as the possible next one,
		   we need to mask out previous cpus */
		if (HWLOC_CPUSUBSET_INDEX(prev_cpu) == i)
			w &= ~((HWLOC_CPUSUBSET_VAL(prev_cpu) << 1) - 1);

		if (w)
			return hwloc_ffsl(w) - 1 + HWLOC_BITS_PER_LONG*i;
	}

	return -1;
}

void hwloc_cpuset_singlify(struct hwloc_cpuset_s * set)
{
	int i,found = 0;

	HWLOC__CPUSET_CHECK(set);

	for(i=0; i<set->ulongs_count; i++) {
		if (found) {
			HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_ZERO;
			continue;
		} else {
			/* subsets are unsigned longs, use ffsl */
			unsigned long w = HWLOC_CPUSUBSET_SUBSET(*set,i);
			if (w) {
				int _ffs = hwloc_ffsl(w);
				HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_VAL(_ffs-1);
				found = 1;
			}
		}
	}

	if (set->infinite) {
		if (found) {
			set->infinite = 0;
		} else {
			/* set the first non allocated bit */
			unsigned first = set->ulongs_count * HWLOC_BITS_PER_LONG;
			set->infinite = 0; /* do not let realloc fill the newly allocated sets */
			hwloc_cpuset_set(set, first);
		}
	}
}

int hwloc_cpuset_compare_first(const struct hwloc_cpuset_s * set1, const struct hwloc_cpuset_s * set2)
{
	int i;

	HWLOC__CPUSET_CHECK(set1);
	HWLOC__CPUSET_CHECK(set2);

	for(i=0; i<set1->ulongs_count || i<set2->ulongs_count; i++) {
		unsigned long w1 = i<set1->ulongs_count ? HWLOC_CPUSUBSET_SUBSET(*set1,i) : set1->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
		unsigned long w2 = i<set2->ulongs_count ? HWLOC_CPUSUBSET_SUBSET(*set2,i) : set2->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
		if (w1 || w2) {
			int _ffs1 = hwloc_ffsl(HWLOC_CPUSUBSET_SUBSET(*set1,i));
			int _ffs2 = hwloc_ffsl(HWLOC_CPUSUBSET_SUBSET(*set2,i));
			/* if both have a bit set, compare for real */
			if (_ffs1 && _ffs2)
				return _ffs1-_ffs2;
			/* one is empty, and it is considered higher, so reverse-compare them */
			return _ffs2-_ffs1;
		}
	}
	if ((!set1->infinite) != (!set2->infinite))
		return !!set1->infinite - !!set2->infinite;
	return 0;
}

int hwloc_cpuset_compare(const struct hwloc_cpuset_s * set1, const struct hwloc_cpuset_s * set2)
{
	unsigned long val;
	int i;

	HWLOC__CPUSET_CHECK(set1);
	HWLOC__CPUSET_CHECK(set2);

	if ((!set1->infinite) != (!set2->infinite))
		return !!set1->infinite - !!set2->infinite;

	val = set2->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(i=set1->ulongs_count-1; i>=set2->ulongs_count; i--) {
		if (HWLOC_CPUSUBSET_SUBSET(*set1,i) == val)
			continue;
		return HWLOC_CPUSUBSET_SUBSET(*set1,i) < val ? -1 : 1;
	}

	val = set1->infinite ? HWLOC_CPUSUBSET_FULL : HWLOC_CPUSUBSET_ZERO;
	for(i=set2->ulongs_count-1; i>=set1->ulongs_count; i--) {
		if (val == HWLOC_CPUSUBSET_SUBSET(*set2,i))
			continue;
		return val < HWLOC_CPUSUBSET_SUBSET(*set2,i) ? -1 : 1;
	}

	for(i=(set2->ulongs_count > set1->ulongs_count ? set1->ulongs_count : set2->ulongs_count)-1; i>=0; i--) {
		if (HWLOC_CPUSUBSET_SUBSET(*set1,i) == HWLOC_CPUSUBSET_SUBSET(*set2,i))
			continue;
		return HWLOC_CPUSUBSET_SUBSET(*set1,i) < HWLOC_CPUSUBSET_SUBSET(*set2,i) ? -1 : 1;
	}

	return 0;
}

int hwloc_cpuset_weight(const struct hwloc_cpuset_s * set)
{
	int weight = 0;
	int i;

	HWLOC__CPUSET_CHECK(set);

	if (set->infinite)
		/* TODO: document */
		return -1;

	for(i=0; i<set->ulongs_count; i++)
		weight += hwloc_weight_long(HWLOC_CPUSUBSET_SUBSET(*set,i));
	return weight;
}
