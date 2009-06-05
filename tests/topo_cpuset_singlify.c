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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* check topo_cpuset_singlify() */

int main()
{
  topo_cpuset_t orig, expected;

  /* empty set gives empty set */
  topo_cpuset_zero(&orig);
  topo_cpuset_singlify(&orig);
  topo_cpuset_zero(&expected);
  assert(!topo_cpuset_compar(&orig, &expected));

  /* full set gives first bit only */
  topo_cpuset_fill(&orig);
  topo_cpuset_singlify(&orig);
  topo_cpuset_zero(&expected);
  topo_cpuset_set(&expected, 0);
  assert(!topo_cpuset_compar(&orig, &expected));

  /* actual non-trivial set */
  topo_cpuset_zero(&orig);
  topo_cpuset_set(&orig, 45);
  topo_cpuset_set(&orig, 46);
  topo_cpuset_set(&orig, 517);
  topo_cpuset_singlify(&orig);
  topo_cpuset_zero(&expected);
  topo_cpuset_set(&expected, 45);
  assert(!topo_cpuset_compar(&orig, &expected));

  return 0;
}
