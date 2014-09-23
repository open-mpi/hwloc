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

use JSON;
use JSON::Schema;

########################################################################
#
########################################################################
my $schema = `cat netloc_reader_static_schema.json`;

my $validator = JSON::Schema->new($schema);
my $json;
my $result;

foreach my $file (@ARGV) {
  print "Checking: $file -- \t";

  $json = `cat $file`;

  $result = $validator->validate($json);

  if( $result) {
    print "Valid\n";
  } else {
    print "Invalid:\n";
    print "\t - $_\n" foreach $result->errors;
  }
}

exit 0;
