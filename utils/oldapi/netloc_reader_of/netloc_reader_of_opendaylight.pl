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
my $default_username = "admin";
my $default_password = "admin";

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
    # Connect to the OpenDaylight controller
    #
    #print "Reading from OpenDaylight controller on subnet $subnet_id\n";

    my $client = REST::Client->new();

    #
    # Default 127.0.0.1:8080 with basic authentication
    #
    $client->setHost("http://" . $current_uri_addr);
    $headers = {Accept => 'application/json',
                Authorization => 'Basic ' . encode_base64($current_username . ':' . $current_password)};

    #
    # Topology information (Switch detailed information)
    # Used to access the list of switches
    #
    $path = "/controller/nb/v2/switchmanager/default/nodes";
    $client->GET( $path, $headers );
    if( $client->responseCode() != 200 ) {
        print "Error: Failed to open the following URI (code = " . $client->responseCode() . "):\n";
        print "\thttp://" . $current_uri_addr . $path . "\n";
        return -1;
    }

    my %data_hash = %{from_json($client->responseContent())};

    while( my $key = each %data_hash) {
        my @edges_array = @{$data_hash{$key}};
        foreach my $value (@edges_array) {
            my %switch_hash = %{$value};

            my $dpid = $switch_hash{"node"}{"id"};
            my $host = new Node;

            $host->type("SW");
            # dpid functions as both the logical and physical id for switches
            $host->log_id( $dpid );
            $host->phy_id( $dpid );
            $host->subnet_id( $subnet_id );
            $host->network_type("ETH");
            $host->description(" ");

            $nodes{$host->phy_id} = $host;
        }
    }

    #
    # Topology information (Switches and swtich connections)
    # Used to access the list of edges between switches
    #
    $path = "/controller/nb/v2/topology/default";
    $client->GET( $path, $headers );
    if( $client->responseCode() != 200 ) {
        print "Error: Failed to open the following URI (code = " . $client->responseCode() . "):\n";
        print "\thttp://" . $current_uri_addr . $path . "\n";
        return -1;
    }

    %data_hash = %{from_json($client->responseContent())};

    while( my $key = each %data_hash) {
        my @edges_array = @{$data_hash{$key}};
        foreach my $value (@edges_array) {
            my %edges_hash = %{$value};

            my $src_dpid  = $edges_hash{"edge"}{"headNodeConnector"}{"node"}{"id"};
            my $src_port  = $edges_hash{"edge"}{"headNodeConnector"}{"id"};
            my $dest_dpid = $edges_hash{"edge"}{"tailNodeConnector"}{"node"}{"id"};
            my $dest_port = $edges_hash{"edge"}{"tailNodeConnector"}{"id"};

            my $bw = $default_speed;
            if( exists $edges_hash{"properties"}{"bandwidth"} ) {
                $bw = sprintf("%d", $edges_hash{"properties"}{"bandwidth"}{"value"} );
            }

            my $desc = " ";
            if( exists $edges_hash{"properties"}{"name"} ) {
                $desc = $edges_hash{"properties"}{"name"};
            }

            # From what I can tell this is not supposed to be a SW <-> SW edge, but
            # is just another reference for a switch to host connection.
            # Really just want to ignore these edges for now, and pick them up below
            # JJH: Unfortunately, that means we do not get the bandwidth and description information.
            my $src_type = "SW";
            if( $edges_hash{"edge"}{"headNodeConnector"}{"type"} eq "PR" ) {
                $dest_port = "-1";
                $src_type = "CA";
                next;
            }

            my $dest_type = "SW";
            if( $edges_hash{"edge"}{"tailNodeConnector"}{"type"} eq "PR" ) {
                $dest_port = "-1";
                $dest_type = "CA";
                next;
            }

            push(@{ $nodes{ $src_dpid }->connections->{ $dest_dpid } },
                 new Edge(
                     port_from      => $src_dpid,
                     port_id_from   => sprintf("%d", $src_port),
                     port_type_from => $src_type,
                     width          => $default_width,
                     speed          => $bw,
                     gbits          => $bw+0, # XXX
                     port_to        => $dest_dpid,
                     port_id_to     => sprintf("%d", $dest_port),
                     port_type_to   => $dest_type,
                     description    => $desc
                 )
                );
        }
    }

    #
    # Topology information (Hosts)
    # Used to access host information and how they are connected to switches
    #
    # Note: we should probably also grab the 'inactive' hosts
    #
    $path = "/controller/nb/v2/hosttracker/default/hosts/active";
    $client->GET($path, $headers);
    if( $client->responseCode() != 200 ) {
        print "Error: Failed to open the following URI (code = " . $client->responseCode() . "):\n";
        print "\thttp://" . $current_uri_addr . $path . "\n";
        return -1;
    }

    %data_hash = %{from_json($client->responseContent())};

    while( my $key = each %data_hash) {
        my @hosts_array = @{$data_hash{$key}};
        foreach my $value (@hosts_array) {
            my %hosts_hash = %{$value};

            my $mac = $hosts_hash{"dataLayerAddress"};
            my $ip = "";
            if( exists $hosts_hash{"networkAddress"} && defined  $hosts_hash{"networkAddress"} ) {
                $ip = $hosts_hash{"networkAddress"};
            }
            # Not used: $hosts_hash{"vlan"}

            my $host = new Node;

            $host->type("CA");
            $host->log_id( $ip );
            $host->phy_id( $mac );
            $host->subnet_id( $subnet_id );
            $host->network_type("ETH");
            $host->description(" ");

            $nodes{$host->phy_id} = $host;

            #
            # Bi-directional link between the node and the switch
            #
            my $dpid = $hosts_hash{"nodeId"};
            my $switch_ref = $nodes{ $dpid };

            # Assume bi-directional link
            # switch -> host
            push(@{$switch_ref->connections->{$host->phy_id}},
                 new Edge(
                     port_from      => $switch_ref->phy_id,
                     port_id_from   => sprintf("%d", $hosts_hash{"nodeConnectorId"} ),
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

            # host -> switch
            push(@{$host->connections->{$switch_ref->phy_id}},
                 new Edge(
                     port_from      => $host->phy_id,
                     port_id_from   => "-1",
                     port_type_from => "CA",
                     width          => $default_width,
                     speed          => $default_speed,
                     gbits          => $default_gbits,
                     port_to        => $switch_ref->phy_id,
                     port_id_to     => sprintf("%d", $hosts_hash{"nodeConnectorId"} ),
                     port_type_to   => "SW",
                     description    => " "
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

    return 0;
}

########################################################################
