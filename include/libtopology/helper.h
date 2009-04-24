/* Copyright 2009 INRIA, Université Bordeaux 1  */

/* Miscellaneous topology helpers to be used the libtopology's core.  */

#ifndef LIBTOPOLOGY_HELPER_H
#define LIBTOPOLOGY_HELPER_H

#define lt_set_empty_os_numbers(l) do { \
		struct lt_level *___l = (l); \
		___l->os_node = -1; \
		___l->os_die  = -1;  \
		___l->os_l3   = -1;  \
		___l->os_l2   = -1;  \
		___l->os_core = -1; \
		___l->os_l1   = -1;  \
		___l->os_cpu  = -1;  \
	} while(0)

#define lt_set_os_numbers(l, _field, _val) do { \
		struct lt_level *__l = (l); \
		lt_set_empty_os_numbers(l); \
		__l->os_##_field = _val; \
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
		__l1->os_node = -1;					\
		__l1->os_die = -1;					\
		__l1->os_l3 = -1;					\
		__l1->os_l2 = -1;					\
		__l1->os_core = -1;					\
		__l1->os_l1 = -1;					\
		__l1->os_cpu = -1;					\
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
		__t->levels[0] = malloc (sizeof (struct lt_level));	\
		lt_setup_machine_level (&(__t->levels[0]));		\
  } while (0)

#endif /* LIBTOPOLOGY_HELPER_H */
