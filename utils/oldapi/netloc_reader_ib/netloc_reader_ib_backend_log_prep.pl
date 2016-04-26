#!/usr/bin/perl

#
# Copyright © 2013      University of Wisconsin-La Crosse.
#                         All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#

use strict;
use warnings;

use Getopt::Long;

use JSON;

use lib "@bindir@";

use Perl_IB_support;

main(@ARGV);

########################################################################
####                   Public Functions                             ####
########################################################################

sub main {
    my $file_dir_arg;
    my $outfile_arg;

    &Getopt::Long::Configure("bundling");
    my $ok = Getopt::Long::GetOptions(
                "indir|d=s" => \$file_dir_arg,
                "outfile|o=s" => \$outfile_arg
            );

    if( !$ok ) {
        print "Error: Must Specify --indir|-d, and --outfile|-o\n";
        exit(1);
    }
    elsif( !defined($file_dir_arg) || length($file_dir_arg) <= 0 ) {
        print "Error: Must specify an input directory (contining ibroutes data files)\n";
        exit(1);
    }
    elsif( !defined($outfile_arg) || length($outfile_arg) <= 0 ) {
        print "Error: Must specify an output filename\n";
        exit(1);
    }

    _logical_prep($file_dir_arg . "/", $outfile_arg);
}

########################################################################
####                   Private Functions                            ####
########################################################################

sub _logical_prep {
    my $dir     = shift(@_);
    my $outfile = shift(@_);

    if(-e $outfile) {
        print "\tLogical pathfinding prep file already exists ($outfile)\n";
        return;
    }

    my %routing_table;

    #
    # Read in all routing information
    #
    die "Cannot read $dir!\n"
        unless -r $dir;

    opendir(my $input_dh, $dir)
        or die "Error: Failed to open $dir\n";

    ITER_DIR:
    while (my $file_name = readdir($input_dh)) {
        if($file_name =~ m/^\.\.?$/) {
            next;
        }

        _process_logical_route_file($dir . $file_name, \%routing_table);
    }
    closedir($input_dh);

    open(my $fh, ">", $outfile);
    print $fh to_json( \%routing_table, {allow_blessed=>1,convert_blessed=>1} );
    close($fh);
}

########################################################################

sub _process_logical_route_file {
    die "\"_process_logical_route_file\" usage: <file name> <global route table>\n"
        unless (scalar(@_) == 2)    and
            "" eq ref $_[0]         and
            ref $_[1] eq "HASH";

    my ($file_name, $global_route_table) = @_;

    my ($guid, $lid);
    my %routes;

    unless (-r $file_name) {
        die "Cannot read \"$file_name\"\n";
        return;
    }

    open(my $input_fh, "<", $file_name)
        or die "Error: Failed to open $file_name\n";

    ITER_INPUT_FILE:
    while(<$input_fh>) {
        next ITER_INPUT_FILE
            unless m/^Unicast/i or m/^0x[0-9a-f]{4}/i;

        my $line = $_;

        #
        # Parse the individual routes
        # Looks like:
        # 0x0023 013 : (Channel Adapter portguid 0x0005ad000008bd05: 'Description...')
        #   Lid  Out   Destination Info
        #        Port
        #
        if($line =~ m/^(0x[0-9a-f]{4}) (\d{3})/) {
            #print "\tRoute: (".$1.") (".hex($1).") (".$2.")\n";
            # Parse route
            my $log_id = hex($1);
            my $port_id = $2;
            $port_id =~ s/^0+//;
            $routes{$log_id} = $port_id;
        }
        #
        # Grab info from top line of the file
        # Looks like:
        # Unicast lids [0x0-0x6b] of switch Lid 21 guid 0x0005ad00070429f6 (Description...)
        #
        elsif($line =~ m/^Unicast lids \[0x[0-9a-f]+-0x[0-9a-f]+] of switch Lid (\d+) guid (0x[0-9a-f]{16})/i) {
            $lid  = $1;
            $guid = $2;
            #print "Routes for ".$guid." ".$lid."\n";
        }
        else {
            die "Unexpected format: \"$line\"\n";
        }
    }

    close($input_fh);

    $global_route_table->{phy_id_normalizer($guid)} = \%routes;
}

########################################################################
