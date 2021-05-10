#!/usr/bin/env perl
#
# Copyright © 2021 Inria.  All rights reserved.
# $COPYRIGHT$
#
#
# For any option found in usage (either .c (C file) or .in (Shell script))
# check that they appear in the manpage (dash escaped) and completions.
#
# By default, added lines in git diff HEAD are considered for listing options.
# -a enables all options in usage.
#

use strict;
use Getopt::Long;
use Cwd;

my $verbose = 0;
my $checkalllines = 0;
my $help = 0;

GetOptions(
    "verbose" => \$verbose,
    "all" => \$checkalllines,
    "help" => \$help,
) or die "Unrecognized options.";

if ($help) {
    print <<EOT;
$0 [options]

--all | -a           Check all lines, not only added ones
--verbose | -v       Verbose messages
--help | -h          Show this help message
EOT
    exit(0);
}

# Go to the root of the GIT clone
while (! -e ".git") {
    chdir("..");
    die "Can't find root of the GIT clone" if (cwd() eq "/");
}

# Load the completion file only once
my $completion_file = "contrib/completion/bash/hwloc";
open(COMPL, $completion_file) or die "couldn't open completions $completion_file: $!";
my @completion_lines = <COMPL>;
close COMPL;

# If >1, we found some missing options
my $missing = 0;

# Check that one option is in the manpage
sub check_manpage {
    my ($file, $_option) = @_;

    # escape dashes
    my $option = $_option;
    $option =~ s/-/\\\\-/g;

    open(MAN, $file)
	or die "couldn't open manpage $file: $!";

    my $found = 0;
    while (<MAN>) {
	if ($_ =~ /$option[ =\\]/) { # option followed by space, equal or \fR
	    $found = 1;
	    last;
	}
    }
    close MAN;

    if (!$found) {
	print "  Failed to find $_option in manpage $file\n";
	$missing++;
    }
}

# Extract the completion function for one program
sub find_completion_func {
    my ($func, $option) = @_;

    my $start = undef;
    my $end = undef;
    for(my $i=0;; $i++) {
	if ($completion_lines[$i] =~ /^$func\(\)\s*\{$/) {
	    $start = $i+1;
	} elsif ($completion_lines[$i] =~ /^\}/ and defined $start) {
	    $end = $i-1;
	    last;
	}
    }
    die "Failed to find beginning of completion function $func in $completion_file"
	unless defined $start;
    die "Failed to find end of completion function $func in $completion_file"
	unless defined $end;
    return @completion_lines[$start..$end];
}

# Check that one option is in the completion function
sub check_completion {
    my ($func, $option, @lines) = @_;
    if (!grep { /$option/ } @lines) {
	print "  Failed to find $option in completion function $func in $completion_file\n";
	$missing++;
    }
}

# Check one program
sub check_file {
    my ($source_file, $manpage_file, $completion_func) = @_;

    if ($verbose) {
	print "Checking changes in $source_file for manpage $manpage_file and completion function $completion_func\n";
    }

    # C or Shell file?
    my $is_c_file = 0;
    $is_c_file = 1 if $source_file =~ /\.c$/;
    my $is_shell_file = 0;
    $is_shell_file = 1  if $source_file =~ /\.in$/;
    die "Source file $source_file is neither C nor shell?" unless $is_shell_file + $is_c_file == 1;

    my @lines;
    if ($checkalllines) {
	# Read the entire file
	open (SOURCE, $source_file) or die "Couldn't open source $source_file: $!";
	@lines = <SOURCE>;
	close SOURCE;
    } else {
	# Only read added lines
	open(SOURCE, "git diff HEAD -- $source_file |") or die "Couldn't open git diff of $source_file: $!";
	while (<SOURCE>) {
	    my $line = $_;
	    next unless $line =~ /^\+/;
	    $line =~ s/^\+//;
	    push @lines, $line;
	}
	close SOURCE;
    }

    # Only keep usage lines
    if ($is_c_file) {
	# fprintf(where, "  -...
	my $pat = qr/^\s+fprintf\s*\(where,\s*"\s+-/;
	@lines = grep { /$pat/ } @lines;
    } elsif ($is_shell_file) {
	# echo "  -...
	my $pat = qr/^\s+echo\s+"\s+-/;
	@lines = grep { /$pat/ } @lines;
    }

    # Extract options from those lines
    my @options = ();
    foreach my $line (@lines) {
	my @tokens = split /\s+/, $line;
	foreach my $token (@tokens) {
	    next unless $token =~ /^-/;
	    $token =~ s/[=\[\"\\].*//; # stop after '=' (mandatory arg) '[' (optional arg) or '\' (\n) or '"' (multiple lines)
	    push @options, $token;
	}
    }

    # Extract the completion function for this program
    my @complines;
    if (defined $completion_func) {
	@complines = find_completion_func ($completion_func);
	return unless @complines;
    }

    # Check every option
    foreach my $option (@options) {
	if ($verbose) {
	    print " Looking for option $option ...\n";
	}
	check_manpage ($manpage_file, $option);
	if (defined $completion_func) {
	    check_completion ($completion_func, $option, @complines);
	}
    }
}

check_file ("utils/lstopo/lstopo.c", "utils/lstopo/lstopo-no-graphics.1in", "_lstopo");
check_file ("utils/hwloc/hwloc-annotate.c", "utils/hwloc/hwloc-annotate.1in", "_hwloc_annotate");
check_file ("utils/hwloc/hwloc-bind.c", "utils/hwloc/hwloc-bind.1in", "_hwloc_bind");
check_file ("utils/hwloc/hwloc-calc.c", "utils/hwloc/hwloc-calc.1in", "_hwloc_calc");
check_file ("utils/hwloc/hwloc-diff.c", "utils/hwloc/hwloc-diff.1in", "_hwloc_diff");
check_file ("utils/hwloc/hwloc-distrib.c", "utils/hwloc/hwloc-distrib.1in", "_hwloc_distrib");
check_file ("utils/hwloc/hwloc-dump-hwdata.c", "utils/hwloc/hwloc-dump-hwdata.1in"); # no completion
check_file ("utils/hwloc/hwloc-gather-cpuid.c", "utils/hwloc/hwloc-gather-cpuid.1in", "_hwloc_gather_cpuid");
check_file ("utils/hwloc/hwloc-info.c", "utils/hwloc/hwloc-info.1in", "_hwloc_info");
check_file ("utils/hwloc/hwloc-patch.c", "utils/hwloc/hwloc-patch.1in", "_hwloc_patch");
check_file ("utils/hwloc/hwloc-ps.c", "utils/hwloc/hwloc-ps.1in", "_hwloc_ps");

check_file ("utils/hwloc/hwloc-gather-topology.in", "utils/hwloc/hwloc-gather-topology.1in", "_hwloc_gather_topology");
check_file ("utils/hwloc/hwloc-compress-dir.in", "utils/hwloc/hwloc-compress-dir.1in", "_hwloc_compress_dir");

exit 1 if $missing;

exit 0
