#!/usr/bin/perl

# Copyright © 2013      University of Wisconsin-La Crosse.
#                         All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#

use strict;
use warnings;

use Data::Dumper;
use MIME::Base64;
use REST::Client;
use JSON;

use Getopt::Long;

use lib "@bindir@";

use Perl_OF_support;

#
# JJH:
# Below are used as default/dummy values if we cannot detect
# similar values for this network.
#
my $default_speed = "1";
my $default_width = "1";
my $default_gbits = 1;

my $default_uri_addr = "127.0.0.1:8080";
my $default_username = "";
my $default_password = "";

my $current_uri_addr = $default_uri_addr;
my $current_username = $default_username;
my $current_password = $default_password;

main();

########################################################################
####                   Public Functions                             ####
########################################################################

sub main {
    my $subnet_id_arg;
    my $outfile_arg;
    my $address_arg;
    my $username_arg;
    my $password_arg;

    &Getopt::Long::Configure("bundling");
    my $ok = Getopt::Long::GetOptions(
                "subnet|s=s"   => \$subnet_id_arg,
                "outfile|o=s"  => \$outfile_arg,
                "addr|a=s"     => \$address_arg,
                "username|u=s" => \$username_arg,
                "password|p=s" => \$password_arg
            );

    if( !$ok ) {
        print "Error: Must Specify --subnet|-s, and --outfile|-o\n";
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

    if( defined($address_arg) && length($address_arg) > 0 ) {
        $current_uri_addr = $address_arg;
    }
    if( defined($username_arg) && length($username_arg) > 0 ) {
        $current_username = $username_arg;
    }
    if( defined($password_arg) && length($password_arg) > 0 ) {
        $current_password = $password_arg;
    }

    my $ret = _parse_network_topology($subnet_id_arg, $outfile_arg);
    if( $ret != 0 ) {
        exit($ret);
    }
}

########################################################################
####                   Private Functions                            ####
########################################################################

sub _parse_network_topology {
    my $subnet_id = shift(@_);
    my $outfile   = shift(@_);

    my $path;
    my $headers;

    my (%nodes);

    #
    # Connect to the Floodlight controller
    #
    #print "Reading from Floodlight controller on subnet $subnet_id\n";

    my $client = REST::Client->new();

    #
    # Default 127.0.0.1:8080 with no authentication
    #
    $client->setHost("http://" . $current_uri_addr);
    if( defined($current_username) && length($current_username) > 0 ) {
        $headers = {Accept => 'application/json',
                    Authorization => 'Basic ' . encode_base64($current_username . ':' . $current_password)};
    }

    #
    # Get switch DPIDS
    #
    $path = "/wm/topology/switchclusters/json";
    $client->GET( $path, $headers );
    if( $client->responseCode() != 200 ) {
        print "Error: Failed to open the following URI (code = " . $client->responseCode() . "):\n";
        print "\thttp://" . $current_uri_addr . $path . "\n";
        return -1;
    }

    # Works correctly when mn --topo is tree. Rest untested.
    my %dpids_hash = %{from_json($client->responseContent())};
    my @dpid_list;
    while( my $key = each %dpids_hash) {
        push(@dpid_list, @{$dpids_hash{$key}});
    }

    #
    # Gather the switches
    #
    foreach my $dpid (@dpid_list) {
        my $port = new Node;

        $port->type("SW");
        # dpid functions as both the logical and physical id
        $port->log_id($dpid);
        $port->phy_id($dpid);
        $port->subnet_id($subnet_id);
        $port->network_type("ETH");
        $port->description(" ");

        $nodes{$dpid} = $port;
    }

    #
    # Get inter switch edges
    #
    $path = "/wm/topology/links/json";
    $client->GET( $path, $headers );
    if( $client->responseCode() != 200 ) {
        print "Error: Failed to open the following URI (code = " . $client->responseCode() . "):\n";
        print "\thttp://" . $current_uri_addr . $path . "\n";
        return -1;
    }

    # Build inter-switch edges
    my @links = @{from_json($client->responseContent())};
    foreach my $hash_ref (@links) {
        push(@{ $nodes{ $hash_ref->{"src-switch"} }->connections->{$hash_ref->{"dst-switch"}} },
            new Edge(
                port_from      => $hash_ref->{"src-switch"},
                port_id_from   => sprintf("%d", $hash_ref->{"src-port"}),
                port_type_from => "SW",
                width          => $default_width,
                speed          => $default_speed,
                gbits          => $default_gbits,
                port_to        => $hash_ref->{"dst-switch"},
                port_id_to     => sprintf("%d", $hash_ref->{"dst-port"}),
                port_type_to   => "SW",
                description    => " "
            )
        );
    }

    #
    # Get device info
    #
    $path = "/wm/device/";
    $client->GET( $path, $headers );
    if( $client->responseCode() != 200 ) {
        print "Error: Failed to open the following URI (code = " . $client->responseCode() . "):\n";
        print "\thttp://" . $current_uri_addr . $path . "\n";
        return -1;
    }

    # Build list of hosts and host-switch edges

    my @devs = @{from_json($client->responseContent())};
    foreach my $hash_ref (@devs) {
        # Some entries are listed without an attachment point
        if(scalar(@{$hash_ref->{"attachmentPoint"}}) > 0) {

            my $host = new Node;

            $host->type("CA");
            # mac is an array. Currently assumed to only ever have one
            #  entry
            $host->phy_id($hash_ref->{"mac"}->[0]);
            # ipv4 is an array. Currently only used if it has a single
            #  entry, else the phy_id is used
            if(scalar(@{$hash_ref->{"ipv4"}}) == 1) {
                $host->log_id($hash_ref->{"ipv4"}->[0]);
            }
            else {
                $host->log_id("");
            }
            $host->subnet_id($subnet_id);
            $host->network_type("ETH");
            $host->description(" ");

            # attachment point is an array, add all the edges
            foreach my $point_ref (@{$hash_ref->{"attachmentPoint"}}) {
                my $switch = $nodes{$point_ref->{"switchDPID"}};

                push(@{$switch->connections->{$host->phy_id}},
                    new Edge(
                        port_from      => $switch->phy_id,
                        port_id_from   => sprintf("%d", $point_ref->{"port"}),
                        port_type_from => "SW",
                        width          => $default_width,
                        speed          => $default_speed,
                        gbits          => $default_gbits,
                        port_to        => $host->phy_id,
                        port_id_to     => "-1",
                        port_type_to   => "CA",
                        description    => " "
                    )
                );
                push(@{$host->connections->{$switch->phy_id}},
                    new Edge(
                        port_from      => $host->phy_id,
                        port_id_from   => "-1",
                        port_type_from => "CA",
                        width          => $default_width,
                        speed          => $default_speed,
                        gbits          => $default_gbits,
                        port_to        => $switch->phy_id,
                        port_id_to     => sprintf("%d", $point_ref->{"port"}),
                        port_type_to   => "SW",
                        description    => " "
                    )
                );
            }

            $nodes{$host->phy_id} = $host;
        }
    }

    #
    # Write data to the file
    #
    open(my $fh_s, ">", $outfile)
        or die "Error: Failed to open $outfile\n";

    print $fh_s to_json( \%nodes, {allow_blessed=>1,convert_blessed=>1} );

    close($fh_s);

    return 0;
}

########################################################################
