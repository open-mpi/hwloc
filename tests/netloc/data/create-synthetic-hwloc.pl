#!/usr/bin/perl

#
# Copyright © 2014      University of Wisconsin-La Crosse.
#                         All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#
#  ./create-synthetic-hwloc.pl -t node-template.xml -o hwloc/node02.xml -h node02 -e "00:00:00:00:00:01"
#

use strict;
use warnings;

use Getopt::Long;

########################################################################
# Process Command Line Arguments
########################################################################
my $KEY_HOSTNAME = "TEMPLATE_HOSTNAME";
my $KEY_MAC_ADDR = "TEMPLATE_MAC_ADDR";

my $template_arg;
my $output_arg;
my $hostname_arg;
my $eth_mac_arg;

&Getopt::Long::Configure("bundling");
my $ok = Getopt::Long::GetOptions(
                "template|t=s"  => \$template_arg,
                "hostname|h=s"  => \$hostname_arg,
                "ethmac|e=s"    => \$eth_mac_arg,
                "output|o=s"    => \$output_arg
            );

if( !$ok ) {
  print "Error: Must specify --template|-t, --output|-o, --hostname|-h, and --ethmac|-e\n";
  exit(-1);
}
elsif( !defined($template_arg) || length($template_arg) <= 0 ) {
  print "Error: Must specify a template file name.\n";
  exit(-2);
}
elsif( !defined($hostname_arg) || length($hostname_arg) <= 0 ) {
  print "Error: Must specify a string argument to --hostname|-h.\n";
  exit(-2);
}
elsif( !defined($eth_mac_arg) || length($eth_mac_arg) <= 0 ) {
  print "Error: Must specify a string argument to --ethmac|-e.\n";
  exit(-2);
}
elsif( !defined($output_arg) || length($output_arg) <= 0 ) {
  print "Error: Must specify a string argument to --output|-o.\n";
  exit(-2);
}

if( $output_arg eq $template_arg ) {
  print "Error: Cannot overwrite the template\n";
  exit(-3);
}

die "Error: Can't read $template_arg"
  unless(-r $template_arg);

########################################################################
# Create the output file
########################################################################
my $line;

#print "Hostname: <$hostname_arg>\n";
#print "Output  : <$output_arg>\n";
#print "Eth Mac : <$eth_mac_arg>\n";

open(TEMPLATE, "<", $template_arg)
  or die "Error: Failed to open $template_arg\n";

open(OUTPUT, ">", $output_arg)
  or die "Error: Failed to open $output_arg\n";

while( $line = <TEMPLATE> ) {
  if( $line =~ $KEY_HOSTNAME ) {
    $line =~ s/$KEY_HOSTNAME/$hostname_arg/g;
  }
  if( $line =~ $KEY_MAC_ADDR ) {
    $line =~ s/$KEY_MAC_ADDR/$eth_mac_arg/g;
  }
  print OUTPUT "$line";
}

close(TEMPLATE);
close(OUTPUT);

exit 0;
