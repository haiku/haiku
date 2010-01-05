#!/usr/bin/perl

use warnings;
use strict;

=head1 convert_build_config_to_shell_format

This simple script converts its standard input from the jam-specific format
that is being used in BuildSetup into a format that's understood by sh.

=cut

my $data;
while (my $line = <>) {
	$data .= $line;
	if ($data =~ m{\s*([+\w]+)\s*\?=\s*\"?([^;]*?)\"?\s*;}gms) {
		my ($variable, $value) = ($1, $2);
		$variable =~ tr{+}{X};	# '+' is illegal as part of shell variable
		print "$variable='$value'\n";
		$data = '';
	}
}

