#!/usr/bin/perl

#
# Copyright © 2014      University of Wisconsin-La Crosse.
#                         All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#

use strict;
use warnings;

########################################################################
# List all of the tests to run
########################################################################
my @tests = ();

push(@tests, "test_API");
push(@tests, "test_ETH_API");
push(@tests, "test_ETH_verbose");
push(@tests, "test_conv");
push(@tests, "test_find_neighbors");
push(@tests, "test_metadata");

push(@tests, "netloc_hello");
push(@tests, "netloc_nodes");
push(@tests, "netloc_all");

push(@tests, "map_paths data/ node01 1 node08 1");
push(@tests, "lsmap data/");
push(@tests, "test_map_hwloc data/ node02 3");

# JJH the following tests require additional repository access.
#push(@tests, "hwloc_compress");
#push(@tests, "test_map");


########################################################################
# Run all of the tests, stop at first failure
########################################################################
foreach my $binary (@tests) {
  print "-"x50 . "\n";
  print "Running Test: $binary \n";
  print "-"x50 . "\n";

  my $ret = system("./" . $binary);
  if( $ret != 0 ) {
    print "XXXX Failure! Returned $ret!\n";
    exit 1;
  }
  else {
    print "---> Success!\n";
    print "\n";
  }
}

exit 0;
