/* Copyright 2009 INRIA, Université Bordeaux 1  */

#include <libtopology.h>

#include <stdlib.h>
#include <stdio.h>

int
main (int argc, char *argv[])
{
  int err;
  err = topo_init ();
  if (err)
    return EXIT_FAILURE;

  printf ("Hello, world!\n");

  topo_fini ();

  return EXIT_SUCCESS;
}
