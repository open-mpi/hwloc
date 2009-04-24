/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* Miscellaneous topology helpers to be used the libtopology's core.  */

#ifndef LIBTOPOLOGY_HELPER_H
#define LIBTOPOLOGY_HELPER_H

#define lt_set_empty_os_numbers(l) do { \
		struct lt_level *___l = (l); \
		int i; \
		for(i=0; i<LT_LEVEL_MAX; i++) \
		  ___l->physical_index[i] = -1; \
	} while(0)

#define lt_set_os_numbers(l, _type, _val) do { \
		struct lt_level *__l = (l); \
		lt_set_empty_os_numbers(l); \
		__l->physical_index[_type] = _val; \
	} while(0)

#define lt_setup_level(l, _type) do {	       \
		struct lt_level *__l = (l);    \
		__l->type = _type;		\
		lt_cpuset_zero(&__l->cpuset);	\
		__l->arity = 0;			\
		__l->children = NULL;		\
		__l->father = NULL;		\
	} while (0)


#define lt_setup_machine_level(l) do {					\
		struct lt_level **__p = (l);                            \
		struct lt_level *__l1 = &(__p[0][0]);			\
		struct lt_level *__l2 = &(__p[0][1]);			\
		__l1->type = LT_LEVEL_MACHINE;				\
		__l1->level = 0;					\
		__l1->number = 0;					\
		__l1->index = 0;					\
		lt_set_empty_os_numbers(__l1);                          \
		__l1->physical_index[LT_LEVEL_MACHINE] = 0;             \
		lt_cpuset_fill(&__l1->cpuset);				\
		__l1->arity = 0;					\
		__l1->children = NULL;					\
		__l1->father = NULL;					\
		lt_cpuset_zero(&__l2->cpuset);				\
  } while (0)


#define lt_level_cpuset_from_array(l, _value, _array, _max) do { \
		struct lt_level *__l = (l); \
		unsigned int *__a = (_array); \
		int k; \
		for(k=0; k<_max; k++) \
			if (__a[k] == _value) \
				lt_cpuset_set(&__l->cpuset, k); \
	} while (0)


#define lt_setup_topo(t) do {						\
		struct lt_topo * __t = (t);                             \
		__t->nb_processors = 0;					\
		__t->nb_nodes = 0;					\
		__t->nb_levels = 0;					\
		__t->fsys_root_fd = -1;					\
		__t->discovering_level = 1;				\
		__t->level_nbitems[0] = 1;                              \
		__t->levels[0] = malloc (2*sizeof (struct lt_level));	\
		lt_setup_machine_level (&(__t->levels[0]));		\
  } while (0)

#endif /* LIBTOPOLOGY_HELPER_H */
