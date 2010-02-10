#!/usr/bin/perl

use warnings;
use strict;
use File::Basename;
use File::Find;

if (scalar @ARGV < 2) {
	die "Usage: $0 <dir to check> <file to log> <optional number of jobs>\n";
}

if (!-e "./ReadMe.cross-compile") {
	die "ERROR: $0 must be called from Haiku top directory!\n";
}

my %headers;
my $dirToCheck = shift @ARGV;
my $fileToLog = shift @ARGV;
my $jobs = 1;
if (scalar @ARGV) {
	$jobs = shift @ARGV;
}
my $headersDirs;
my $cppcheck;

print "Scanning headers...\n";
find({ wanted => \&process, no_chdir => 1},
	("headers/posix", "headers/libs", "headers/os", "headers/private"));

foreach my $dir (sort keys %headers) {
	$headersDirs .= "-I $dir ";
}

print "Running cppcheck tool...\n";
$cppcheck = "cppcheck -j $jobs --force --auto-dealloc 3rdparty/cppcheck/haiku.lst --enable=exceptRealloc,possibleError $headersDirs $dirToCheck 2> $fileToLog";

system($cppcheck);

sub process
{
	if (substr($_, -4, 4) eq '.svn') {
		$File::Find::prune = 1;
	} else {
		return if $File::Find::dir eq $_;	# skip toplevel folders
		my $name = (fileparse($_))[0];
		if (-d $_) {
			push @{$headers{$File::Find::name}->{subdirs}}, $name;
			return;
		}
	}
}

