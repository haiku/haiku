#!/usr/bin/perl

use warnings;
use strict;

=head1 create_project_files.pl

This simple script traverses the haiku sources and creates (incomplete) *.pro
files in order to make the haiku sources available within the qt-creator IDE.
Additionally, it will add those files to svn:ignore of their parent directory
(unless already contained there).

=cut

use File::Basename;
use File::Find;

if (!@ARGV) {
	die "usage: $0 <haiku-top-path>\n";
}

my $haikuTop = shift @ARGV;
if (!-e "$haikuTop/ReadMe.cross-compile") {
	die "'$haikuTop/ReadMe.cross-compile' not found - not a haiku top!\n";
}

my %collection;

print "scanning ...\n";
find({ wanted => \&process, no_chdir => 1}, 
	("$haikuTop/headers", "$haikuTop/src"));

writeProFile("$haikuTop/haiku.pro", { subdirs => ['headers', 'src'] });
foreach my $dir (sort keys %collection) {
	my $proFile = $dir.'/'.fileparse($dir).'.pro';
	writeProFile($proFile, $collection{$dir});
}

sub process
{
	if (substr($_, -4, 4) eq '.svn') {
		$File::Find::prune = 1;
	} else {
		return if $File::Find::dir eq $_;	# skip toplevel folders
		my $name = (fileparse($_))[0];
		if (-d $_) {
			$collection{$File::Find::dir}->{subdirs} ||= [];
			push @{$collection{$File::Find::dir}->{subdirs}}, $name;
			return;
		}
		elsif ($_ =~ m{\.(h|hpp)$}i) {
			$collection{$File::Find::dir}->{headers} ||= [];
			push @{$collection{$File::Find::dir}->{headers}}, $name;
		}
		elsif ($_ =~ m{\.(c|cc|cpp|s|asm)$}i) {
			$collection{$File::Find::dir}->{sources} ||= [];
			push @{$collection{$File::Find::dir}->{sources}}, $name;
		}
	}
}

sub writeProFile
{
	my ($proFile, $info) = @_;
	
	return if !$info;
	
	print "creating $proFile\n";
	open(my $proFileFH, '>', $proFile)
		or die "unable to write $proFile";
	print $proFileFH "TEMPLATE = subdirs\n";
	print $proFileFH "CONFIG += ordered\n";
	if (exists $info->{subdirs}) {
		print $proFileFH 
			"SUBDIRS = ".join(" \\\n\t", sort @{$info->{subdirs}})."\n";
	}
	if (exists $info->{headers}) {
		print $proFileFH 
			"HEADERS = ".join(" \\\n\t", sort @{$info->{headers}})."\n";
	}
	if (exists $info->{sources}) {
		print $proFileFH 
			"SOURCES = ".join(" \\\n\t", sort @{$info->{sources}})."\n";
	}
	close $proFileFH;
	
	updateSvnIgnore($proFile);
}

sub updateSvnIgnore
{
	my $proFile = shift;

	my ($filename, $parentDir) = fileparse($proFile);

	my $svnIgnore = qx{svn propget --strict svn:ignore $parentDir};
	if (!grep { $_ eq $filename } split "\n", $svnIgnore) {
		chomp $svnIgnore;
		$svnIgnore .= "\n" unless !$svnIgnore;
		$svnIgnore .= "$filename\n";
		open(my $propsetFH, "|svn propset svn:ignore --file - $parentDir")
			or die "unable to open pipe to 'svn propset'";
		print $propsetFH $svnIgnore;
		close($propsetFH);
	}
}
