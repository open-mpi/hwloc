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

/* Wrapper to avoid msys' tendency to turn / into \ and : into ;  */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  char *prog, *arch, *def, *name, *lib;
  char s[1024];

  if (argc != 6) {
    fprintf(stderr,"bad number of arguments");
    exit(EXIT_FAILURE);
  }

  prog = argv[1];
  arch = argv[2];
  def = argv[3];
  name = argv[4];
  lib = argv[5];

  snprintf(s, sizeof(s), "\"%s\" /machine:%s /def:%s /name:%s /out:%s",
      prog, arch, def, name, lib);
  if (system(s)) {
    fprintf(stderr, "%s failed\n", s);
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
