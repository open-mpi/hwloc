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

#include <topology.h>

#include <assert.h>

/* check topo_cpuset_first(), _last() and _weight() */

int main()
{
  topo_cpuset_t set;

  /* empty set */
  topo_cpuset_zero(&set);
  assert(topo_cpuset_first(&set) == -1);
  assert(topo_cpuset_last(&set) == -1);
  assert(topo_cpuset_weight(&set) == 0);

  /* full set */
  topo_cpuset_fill(&set);
  assert(topo_cpuset_first(&set) == 0);
  assert(topo_cpuset_last(&set) == TOPO_NBMAXCPUS-1);
  assert(topo_cpuset_weight(&set) == TOPO_NBMAXCPUS);

  /* custom sets */
  topo_cpuset_zero(&set);
  topo_cpuset_set_range(&set, 36, 59);
  assert(topo_cpuset_first(&set) == 36);
  assert(topo_cpuset_last(&set) == 59);
  assert(topo_cpuset_weight(&set) == 24);
  topo_cpuset_set_range(&set, 136, 259);
  assert(topo_cpuset_first(&set) == 36);
  assert(topo_cpuset_last(&set) == 259);
  assert(topo_cpuset_weight(&set) == 148);
  topo_cpuset_clr(&set, 199);
  assert(topo_cpuset_first(&set) == 36);
  assert(topo_cpuset_last(&set) == 259);
  assert(topo_cpuset_weight(&set) == 147);

  return 0;
}
