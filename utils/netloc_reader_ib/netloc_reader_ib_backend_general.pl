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
    my $infile_arg;
    my $subnet_id_arg;
    my $outfile_arg;

    &Getopt::Long::Configure("bundling");
    my $ok = Getopt::Long::GetOptions(
                "infile|i=s"  => \$infile_arg,
                "subnet|s=s"  => \$subnet_id_arg,
                "outfile|o=s" => \$outfile_arg
            );

    if( !$ok ) {
        print "Error: Must Specify --infile|-i, --subnet|-s, and --outfile|-o\n";
        exit(1);
    }
    elsif( !defined($infile_arg) || length($infile_arg) <= 0 ) {
        print "Error: Must specify an input file (ibnetdiscover data)\n";
        exit(1);
    }
    elsif( !defined($subnet_id_arg) || length($subnet_id_arg) <= 0 ) {
        print "Error: Must specify a subnet string\n";
        exit(1);
    }
    elsif( !defined($outfile_arg) || length($outfile_arg) <= 0 ) {
        print "Error: Must specify an output filename\n";
        exit(1);
    }

    _parse_network_topology($infile_arg, $subnet_id_arg, $outfile_arg);
}

########################################################################
####                   Private Functions                            ####
########################################################################

# Compute gbits: cf https://en.wikipedia.org/wiki/InfiniBand
sub _compute_gbits {
    my $speed;
    my $width;
    my $rate;
    my $encoding;
    my $gb_per_x;
    my $x;
    ($speed, $width) = @_;

    if ($speed eq "SDR") {
        $rate = 2.5;
        $encoding = 8.0/10;
    }
    elsif ($speed eq "DDR") {
        $rate = 5;
        $encoding = 8.0/10;
    }
    elsif ($speed eq "QDR") {
        $rate = 10;
        $encoding = 8.0/10;
    }
    elsif ($speed eq "FDR") {
        $rate = 14.0625;
        $encoding = 64.0/66;
    }
    elsif ($speed eq "FDR10") {
        $rate = 10;
        $encoding = 64.0/66;
    }
    elsif ($speed eq "EDR") {
        $rate = 25;
        $encoding = 64.0/66;
    }
    else {
        return 1;
    }
    $gb_per_x = $rate*$encoding;

    if ($width =~ m/([0-9]*)x/) {
        $x = $1;
    }
    else {
        return 1;
    }

    return sprintf("%f", $x*$gb_per_x);
 }

sub _parse_network_topology {
    my $file_name = shift(@_);
    my $subnet_id = shift(@_);
    my $outfile   = shift(@_);

    my (%nodes);

    #print "Reading file $file_name, subnet ID $subnet_id\n";

    #
    # Open the file
    #
    die "Error: Can't read $file_name"
        unless(-r $file_name);

    open(my $input_fh, "<", $file_name)
        or die "Error: Failed to open $file_name\n";

    #
    # Process the file
    #
    while(<$input_fh>) {
        #
        # Don't need lines beginning with "DR" or artificially inserted
        #  comments
        #
        if(/^DR / || /^\#/) {
            next;
        }

        #
        # Split out the columns. One form of line has both source and
        #  destination information.  The other form only has source
        #  information (because it's not hooked up to anything --
        #  usually a switch port that doesn't have anything plugged in).
        #
        chomp;
        my $line = $_;

        my ($have_peer, $src_type, $src_lid, $src_port_id, $src_guid,
            $width, $speed, $gbits, $dest_type, $dest_lid, $dest_port_id,
            $dest_guid, $edge_desc, $src_desc, $dest_desc);

        #
        # Search for lines that look like:
        # SW    32  2 0x0005ad00070423f6 4x SDR - CA    63  2 0x0005adffff0856b6 ( 'Desc. of src' - 'Desc. of dest.' )
        #
        if ($line !~ m/
                        (CA|SW)          \s+  # Source type
                        (\d+)            \s+  # Source lid
                        (\d+)            \s+  # Source port id
                        (0x[0-9a-f]{16}) \s+  # Source guid

                        (\d+x) \s ([^\s]*) \s+  # Connection width, speed

                        -                \s+  # Dash seperator

                        (CA|SW)          \s+  # Dest type
                        (\d+)            \s+  # Dest lid
                        (\d+)            \s+  # Dest port id
                        (0x[0-9a-f]{16}) \s+  # Dest guid
                        ([\w|\W]+)            # Rest of the line is the description
                    /x) {
            #
            # This is a port that is not connected to anything (no peer information)
            #
            # JJH: We do not need this data, so ignore it for now.
            #
            next;

            $have_peer = 0;

            $line =~ m/(CA|SW)\s+(\d+)\s+(\d+)\s+(0x[0-9a-f]{16})\s+(\d+x)\s([^\s]*)/;

            ($src_type, $src_lid, $src_port_id, $src_guid, $width, $speed) = ($1, $2, $3, $4, $5, $6);
        }
        #
        # Parse the lines that have both a src and dest (active ports)
        #
        else {
            $have_peer = 1;

            ($src_type, $src_lid, $src_port_id, $src_guid, $width, $speed, $dest_type, $dest_lid, $dest_port_id, $dest_guid)
                = ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10);

            # Break up the description
            $edge_desc = $11;
            $edge_desc =~ s/^\s*\(\s*//;
            $edge_desc =~ s/\s*\)\s*$//;

            my @desc_vs = split(" - ", $edge_desc);
            if( scalar(@desc_vs) == 2 ) {
                $src_desc  = $desc_vs[0];
                $dest_desc = $desc_vs[1];
            }
            else {
                $src_desc = "";
                $dest_desc = "";
            }
        }

        #
        # Compute the gbits
        #
        $gbits = _compute_gbits($speed, $width);

        #
        # Get the source node
        #
        my $src_node;
        $src_node = _enter_node(\%nodes, $src_type, $src_lid, $src_guid, $subnet_id, $src_desc);
        print "gbits: $gbits\n";

        #
        # If we have a destination node, get it and link the source to
        #  the destination
        #
        if($have_peer) {

            #
            # Get destination node
            #
            my $dest_node;
            $dest_node = _enter_node(\%nodes, $dest_type, $dest_lid, $dest_guid, $subnet_id, $dest_desc);

            #
            # Add this connection to the list of edges
            #
            push(@{$src_node->connections->{phy_id_normalizer($dest_guid)}},
                new Edge(
                    port_from      => $src_node->phy_id,
                    port_id_from   => $src_port_id,
                    port_type_from => $src_type,
                    width          => $width,
                    speed          => $speed,
                    gbits          => $gbits,
                    port_to        => $dest_node->phy_id,
                    port_id_to     => $dest_port_id,
                    port_type_to   => $dest_type,
                    description    => $edge_desc
                )
            );
        }
    }

    #
    # Write data to the file
    #
    open(my $fh_s, ">", $outfile)
        or die "Error: Failed to open $outfile\n";

    print $fh_s to_json( \%nodes, {allow_blessed=>1,convert_blessed=>1} );

    close($fh_s);
}

########################################################################

#
# Access a node, or add it if has not been seen before
#
sub _enter_node {
    my $nodes_hash_ref = shift(@_);
    my $type           = shift(@_);
    my $log_id         = shift(@_);
    my $phy_id         = shift(@_);
    my $subnet_id      = shift(@_);
    my $desc           = shift(@_);

    my $node;
    # If the port didn't already exist, create and add it
    unless(exists($nodes_hash_ref->{phy_id_normalizer($phy_id)})) {
        $node = new Node;
        $node->type($type);
        $node->log_id($log_id);
        $node->phy_id(phy_id_normalizer($phy_id));
        $node->subnet_id($subnet_id);
        $node->network_type("IB");
        $node->description($desc);

        $nodes_hash_ref->{phy_id_normalizer($phy_id)} = $node;
    }
    # Otherwise, fetch it
    else {
        $node = $nodes_hash_ref->{phy_id_normalizer($phy_id)};
    }

    return $node;
}

########################################################################
