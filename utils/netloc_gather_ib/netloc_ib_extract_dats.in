#!/usr/bin/perl

#
# Copyright © 2013-2016 Inria.  All rights reserved.
# Copyright © 2013-2014 University of Wisconsin-La Crosse.#
#                         All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#

use strict;

use Getopt::Long;

my $workdir;
my $verbose;
my $help;

&Getopt::Long::Configure("bundling");
my $ok = Getopt::Long::GetOptions(
        "verbose|v" => \$verbose,
        "help|h" => \$help
    );

if (!$ok or defined($help) or !defined $ARGV[0]) {
  print "$0 [options] <dir>\n";
  print "  Input raw files must be in <dir>/ib-raw/\n";
  print "  Output files will be stored in <dir>/netloc/\n";
  print "Options:\n";
  print " --verbose | -v\n";
  print "    Verbose\n";
  print " --help | -h\n";
  print "    Help\n";
  exit 1;
}

my $workdir = $ARGV[0];
my $rawdir = "$workdir/ib-raw";
my $outdir = "$workdir/netloc";

my @subnets;
my %ibnetdiscover_files;
my %ibroutes_files;

opendir DIR, $rawdir
  or die "Failed to open raw IB input directory $rawdir\n";
while (my $file = readdir(DIR)) {
  if ($file =~ /^ib-subnet-([0-9a-fA-F:]{19}).txt$/) {
    my $subnet = $1;
    next unless -f "$rawdir/$file";
    next unless -d "$rawdir/ibroutes-$subnet";
    push @subnets, $1;
    $ibnetdiscover_files{$subnet} = "$rawdir/$file";
    $ibroutes_files{$subnet} = "$rawdir/ibroutes-$subnet/";
  }
}
closedir DIR;

if (!-d $outdir) {
  mkdir $outdir
    or die "Cannot create output directory ($!)";
}

###############################################################################
my $cmd;

foreach my $subnet (@subnets) {
  print "-"x70 . "\n";
  print "Processing Subnet: " . $subnet . "\n";
  print "-"x70 . "\n";

  print "-"x15 . " General Network Information\n";
  $cmd = ("netloc_reader_ib ".
          " --subnet ".$subnet.
          " --outdir ".$outdir.
          " --file ".$ibnetdiscover_files{$subnet}.
          " --routedir ".$ibroutes_files{$subnet});
  if( !defined($verbose) ) {
    $cmd .= " 1>/dev/null";
  } else {
    $cmd .= " -p ";
  }
  #print "CMD: $cmd\n";
  system($cmd);
}

exit 0;
