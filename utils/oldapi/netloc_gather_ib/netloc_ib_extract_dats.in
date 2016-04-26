#!/usr/bin/perl

#
# Copyright © 2013 Inria.  All rights reserved.
# Copyright © 2013-2014 University of Wisconsin-La Crosse.#
#                         All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#

use strict;

use Getopt::Long;

my $rawdir = "./ib-raw";
my $outdir = "./netloc";
my $verbose;
my $help;

&Getopt::Long::Configure("bundling");
my $ok = Getopt::Long::GetOptions(
        "out-dir|o=s" => \$outdir,
        "raw-dir|r=s" => \$rawdir,
        "verbose|v" => \$verbose,
        "help|h" => \$help
    );

if (!$ok or !defined $rawdir or defined($help) ) {
  print "Input directory with raw IB data must be specified with\n";
  print " --raw-dir <dir> (default is ./ib-raw)\n";
  print "Output directory for netloc data can be specified with\n";
  print " --out-dir <dir> (default is ./netloc)\n";
  print "Verbose\n";
  print " --verbose | -v\n";
  print "Help\n";
  print " --help | -h\n";
  exit 1;
}

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
