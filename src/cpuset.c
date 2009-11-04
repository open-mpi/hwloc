/*
 * Copyright © 2009 CNRS, INRIA, Université Bordeaux 1
 * Copyright © 2009 Cisco Systems, Inc.  All rights reserved.
 * See COPYING in top-level directory.
 */

#include <private/config.h>
#include <private/cpuset.h>
#include <private/private.h>
#include <hwloc/cpuset.h>

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>


/* overzealous check in debug-mode, not as powerful as valgrind but still useful */
#ifdef HWLOC_DEBUG
#define HWLOC__CPUSET_CHECK(set) assert((set)->magic == HWLOC_CPUSET_MAGIC)
#else
#define HWLOC__CPUSET_CHECK(set)
#endif

int hwloc_snprintf(char *str, size_t size, const char *format, ...)
{
  int ret;
  va_list ap;
  static char bin;

  /* Some systems crash on str == NULL */
  if (!size) {
    str = &bin;
    size = 1;
  }

  va_start(ap, format);
  ret = vsnprintf(str, size, format, ap);
  va_end(ap);

  if (ret >= 0 && ret != size-1)
    return ret;

  /* vsnprintf returned size-1 or -1. That could be a system which reports the
   * written data and not the actually required room. Try increasing buffer
   * size to get the latter. */

  do {
    size *= 2;
    str = malloc(size);
    va_start(ap, format);
    errno = 0;
    ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    free(str);
  } while (ret == size-1 || (ret < 0 && !errno));

  return ret;
}

struct hwloc_cpuset_s * hwloc_cpuset_alloc(void)
{
  struct hwloc_cpuset_s * set;
  set = calloc(sizeof(*set), 1);
#ifdef HWLOC_DEBUG
  if (set)
    set->magic = HWLOC_CPUSET_MAGIC;
#endif
  return set;
}

void hwloc_cpuset_free(struct hwloc_cpuset_s * set)
{
  HWLOC__CPUSET_CHECK(set);
#ifdef HWLOC_DEBUG
  set->magic = 0;
#endif

  free(set);
}

struct hwloc_cpuset_s * hwloc_cpuset_dup(struct hwloc_cpuset_s * old)
{
  struct hwloc_cpuset_s * new;

  HWLOC__CPUSET_CHECK(old);

  new = malloc(sizeof(*new));
  if (new) {
#ifdef HWLOC_DEBUG
    new->magic = HWLOC_CPUSET_MAGIC;
#endif
    memcpy(new, old, sizeof(*new));
  }
  return new;
}

void hwloc_cpuset_copy(struct hwloc_cpuset_s * dst, struct hwloc_cpuset_s * src)
{
  HWLOC__CPUSET_CHECK(dst);
  HWLOC__CPUSET_CHECK(src);

  memcpy(dst, src, sizeof(*dst));
}

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

  i=HWLOC_CPUSUBSET_COUNT-1;
  while (i>=0 || accumed) {
    /* Refill accumulator */
    if (!accumed) {
      accum = set->s[i--];
      accumed = HWLOC_BITS_PER_LONG;
    }

    if (accum & accum_mask) {
      /* print the whole subset if not empty */
      res = hwloc_snprintf(tmp, size, needcomma ? "," HWLOC_PRIxCPUSUBSET : HWLOC_PRIxCPUSUBSET,
		     (accum & accum_mask) >> (HWLOC_BITS_PER_LONG - HWLOC_CPUSET_SUBSTRING_SIZE));
      needcomma = 1;
    } else if (i == -1 && accumed == HWLOC_CPUSET_SUBSTRING_SIZE) {
      /* print a single 0 to mark the last subset */
      res = hwloc_snprintf(tmp, size, needcomma ? ",0" : "0");
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

struct hwloc_cpuset_s * hwloc_cpuset_from_string(const char * __hwloc_restrict string)
{
  struct hwloc_cpuset_s * set;
  char * current = (char *) string;
  int count=0, i;
  unsigned long accum = 0;
  int accumed = 0;

  set = hwloc_cpuset_alloc();
  if (!set)
    return NULL;

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

  return set;
}

void hwloc_cpuset_zero(struct hwloc_cpuset_s * set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_ZERO;
}

void hwloc_cpuset_fill(struct hwloc_cpuset_s * set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_FULL;
}

void hwloc_cpuset_from_ulong(struct hwloc_cpuset_s *set, unsigned long mask)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	HWLOC_CPUSUBSET_SUBSET(*set,0) = mask;
	for(i=1; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) = HWLOC_CPUSUBSET_ZERO;
}

void hwloc_cpuset_from_ith_ulong(struct hwloc_cpuset_s *set, int i, unsigned long mask)
{
	int j;

	HWLOC__CPUSET_CHECK(set);

	HWLOC_CPUSUBSET_SUBSET(*set,i) = mask;
	for(j=1; j<HWLOC_CPUSUBSET_COUNT; j++)
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

	return HWLOC_CPUSUBSET_SUBSET(*set,i);
}

void hwloc_cpuset_cpu(struct hwloc_cpuset_s * set, unsigned cpu)
{
	HWLOC__CPUSET_CHECK(set);

	hwloc_cpuset_zero(set);
	HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) |= HWLOC_CPUSUBSET_VAL(cpu);
}

void hwloc_cpuset_all_but_cpu(struct hwloc_cpuset_s * set, unsigned cpu)
{
	HWLOC__CPUSET_CHECK(set);

	hwloc_cpuset_fill(set);
	HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) &= ~HWLOC_CPUSUBSET_VAL(cpu);
}

void hwloc_cpuset_set(struct hwloc_cpuset_s * set, unsigned cpu)
{
	HWLOC__CPUSET_CHECK(set);

	HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) |= HWLOC_CPUSUBSET_VAL(cpu);
}

void hwloc_cpuset_set_range(struct hwloc_cpuset_s * set, unsigned begincpu, unsigned endcpu)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	for (i=begincpu; i<=endcpu; i++)
		HWLOC_CPUSUBSET_CPUSUBSET(*set,i) |= HWLOC_CPUSUBSET_VAL(i);
}

void hwloc_cpuset_clr(struct hwloc_cpuset_s * set, unsigned cpu)
{
	HWLOC__CPUSET_CHECK(set);

	HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) &= ~HWLOC_CPUSUBSET_VAL(cpu);
}

int hwloc_cpuset_isset(const struct hwloc_cpuset_s * set, unsigned cpu)
{
	HWLOC__CPUSET_CHECK(set);

	return (HWLOC_CPUSUBSET_CPUSUBSET(*set,cpu) & HWLOC_CPUSUBSET_VAL(cpu)) != 0;
}

int hwloc_cpuset_iszero(const struct hwloc_cpuset_s *set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*set,i) != HWLOC_CPUSUBSET_ZERO)
			return 0;
	return 1;
}

int hwloc_cpuset_isfull(const struct hwloc_cpuset_s *set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*set,i) != HWLOC_CPUSUBSET_FULL)
			return 0;
	return 1;
}

int hwloc_cpuset_isequal (const struct hwloc_cpuset_s *set1, const struct hwloc_cpuset_s *set2)
{
	int i;

	HWLOC__CPUSET_CHECK(set1);
	HWLOC__CPUSET_CHECK(set2);

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*set1,i) != HWLOC_CPUSUBSET_SUBSET(*set2,i))
			return 0;
	return 1;
}

int hwloc_cpuset_intersects (const struct hwloc_cpuset_s *set1, const struct hwloc_cpuset_s *set2)
{
	int i;

	HWLOC__CPUSET_CHECK(set1);
	HWLOC__CPUSET_CHECK(set2);

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		if ((HWLOC_CPUSUBSET_SUBSET(*set1,i) & HWLOC_CPUSUBSET_SUBSET(*set2,i)) != HWLOC_CPUSUBSET_ZERO)
			return 1;
	return 0;
}

int hwloc_cpuset_isincluded (const struct hwloc_cpuset_s *sub_set, const struct hwloc_cpuset_s *super_set)
{
	int i;

	HWLOC__CPUSET_CHECK(sub_set);
	HWLOC__CPUSET_CHECK(super_set);

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		if (HWLOC_CPUSUBSET_SUBSET(*super_set,i) != (HWLOC_CPUSUBSET_SUBSET(*super_set,i) | HWLOC_CPUSUBSET_SUBSET(*sub_set,i)))
			return 0;
	return 1;
}

void hwloc_cpuset_orset (struct hwloc_cpuset_s *set, const struct hwloc_cpuset_s *modifier_set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);
	HWLOC__CPUSET_CHECK(modifier_set);

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) |= HWLOC_CPUSUBSET_SUBSET(*modifier_set,i);
}

void hwloc_cpuset_andset (struct hwloc_cpuset_s *set, const struct hwloc_cpuset_s *modifier_set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);
	HWLOC__CPUSET_CHECK(modifier_set);

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) &= HWLOC_CPUSUBSET_SUBSET(*modifier_set,i);
}

void hwloc_cpuset_clearset (struct hwloc_cpuset_s *set, const struct hwloc_cpuset_s *modifier_set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);
	HWLOC__CPUSET_CHECK(modifier_set);

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) &= ~HWLOC_CPUSUBSET_SUBSET(*modifier_set,i);
}

void hwloc_cpuset_xorset (struct hwloc_cpuset_s *set, const struct hwloc_cpuset_s *modifier_set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);
	HWLOC__CPUSET_CHECK(modifier_set);

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		HWLOC_CPUSUBSET_SUBSET(*set,i) ^= HWLOC_CPUSUBSET_SUBSET(*modifier_set,i);
}

int hwloc_cpuset_first(const struct hwloc_cpuset_s * set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++) {
		/* subsets are unsigned longs, use ffsl */
		int _ffs = hwloc_ffsl(HWLOC_CPUSUBSET_SUBSET(*set,i));
		if (_ffs>0)
			return _ffs - 1 + HWLOC_CPUSUBSET_SIZE*i;
	}

	return -1;
}

int hwloc_cpuset_last(const struct hwloc_cpuset_s * set)
{
	int i;

	HWLOC__CPUSET_CHECK(set);

	for(i=HWLOC_CPUSUBSET_COUNT-1; i>=0; i--) {
		/* subsets are unsigned longs, use flsl */
		int _fls = hwloc_flsl(HWLOC_CPUSUBSET_SUBSET(*set,i));
		if (_fls>0)
			return _fls - 1 + HWLOC_CPUSUBSET_SIZE*i;
	}

	return -1;
}

void hwloc_cpuset_singlify(struct hwloc_cpuset_s * set)
{
	int i,found = 0;

	HWLOC__CPUSET_CHECK(set);

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

int hwloc_cpuset_compar_first(const struct hwloc_cpuset_s * set1, const struct hwloc_cpuset_s * set2)
{
	int i;

	HWLOC__CPUSET_CHECK(set1);
	HWLOC__CPUSET_CHECK(set2);

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

int hwloc_cpuset_compar(const struct hwloc_cpuset_s * set1, const struct hwloc_cpuset_s * set2)
{
	int i;

	HWLOC__CPUSET_CHECK(set1);
	HWLOC__CPUSET_CHECK(set2);

	for(i=HWLOC_CPUSUBSET_COUNT-1; i>=0; i--) {
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

	for(i=0; i<HWLOC_CPUSUBSET_COUNT; i++)
		weight += hwloc_weight_long(HWLOC_CPUSUBSET_SUBSET(*set,i));
	return weight;
}
