#!/bin/perl

# headerSampler.pl, Copyright (C) 1997, Mizutori Tetsuya
# <mizutori@nn.iij4u.or.jp>
# January 7, 1997, February 4, 1997.
# Usage:
# Drag and drop the C++ header files onto this 'header.pl' script

use integer;
#use strict ('subs','vars');

my $gMAC = ( $ENV{'HOME'} =~ /^$/ );
#if ( $gMAC ) { print "I love Macintosh.\n"; }
my $gScope = "";
my $gSentence = "";
my $gDocbookpath = "";

# if the command line includes no file list, then prompt the choose dialog.
#push(@ARGV, ChooseFile()) unless defined @ARGV;

DoMain();

ExitToShell();

#

################
# DoMain
################

sub DoMain {
	my $filepath;
	my $countFiles = 0;

	$filepath = $ARGV[0];
	$gDocbookpath = $ARGV[1];
#	foreach $filepath (@ARGV) {
		my ($creator,$type) = GetFileInfo($filepath);

		$countFiles++;
		#print "[$countFiles] = ['$creator','$type'] $filepath\n";

#		skip if the file is not 'TEXT' nor '*.h'.
		next if ($type ne 'TEXT');
		next unless ($filepath =~ /\.h$/);

		$gScope = "private:";
		$gSentence = "";
		@gComment = ();
		$gIsIgnoring = 0;

		open(IN, $filepath) || die "Can't open '$filepath'";
		while (<IN>) {
		chomp;
		HandleTextline();
		}
		close IN;
#	}
}


################
# HandleTextline
################

sub HandleTextline {

# fetch the commant part if it exists.
	if ( m/\/\/.*$/ ) { $gComment[$#gComment + 1] = $&; }

# ignore the comment part in the line.
	s/^(.*)\/\/.*$/$1/;

# trim the line.
	s/^[ \t]+//;
	s/[ \t]+$//;

	SWITCH: {

		# skip 'typedef' or 'struct' region.
		(/^typedef/ || /^struct/ || /^namespace/) && do {
			$gIsIgnoring = 1;
			last SWITCH;
			};

		# skip this line if it begins with "#ifdef", "#endif", and others.
		/^#/ && do {
			@gComment = ();
			last SWITCH;
			};

		# ignore this line if it begins with "public" and other keywords.
		(/^public:/ || /^protected:/ || /^private:/) && do {
			$gScope = $_;
			$gSentence = "";
			@gComment = ();
			last SWITCH;
		};

		# skip this line if it ends with "}???;".
		/\}.*;$/ && do {
			$gSentence = "";
			@gComment = ();
			$gIsIgnoring = 0;
			last SWITCH;
			};

		# skip this line if it ends with "}???". for namespace
		/\}$/ && do {
			$gSentence = "";
			@gComment = ();
			$gIsIgnoring = 0;
			last SWITCH;
			};

		# test if end of 'typedef' or 'struct' region.
			($gIsIgnoring) && do {
			last SWITCH;
			};

		# append this line to my sentence.
		do {
			$gSentence .= "\t" unless $gSentence =~ /^$/;
			$gSentence .= $_;
		};

		# handle my sentence if the line ends with ";".
		/;$/ && do {
			$_ = $gSentence;
			ApplySentence();
			$gSentence = "";
			@gComment = ();
			last SWITCH;
			};
	}
}


################
# ApplySentence
################

sub ApplySentence {
#	my ($textline) = @_;

# eliminate the duplicate spaces to a blank space.
	s/[ \t]+/ /g;

# eliminate the ";" character at the tail end.
	s/[ ]*;$//;

	#if ( s/[ ]*const$// ) { PrintFunction(); }		# if the function has 'const'modifier.
	#elsif ( /\)$/ ) { PrintFunction(); }
	if ( /\)/ ) { PrintFunction(); }
	#else { PrintVariable(); }

}


################
# PrintFunction
################

sub PrintFunction {
# print a comment.
#	my $textline;
#	if ( $#gComment >= 0 ) {
#		foreach $textline (@gComment) {
#			print "\t\t$textline\n";
#		}
#	}

	if (! ($gScope =~ /^public/ )) {
		return;
	}

# print a scope sign.
#print WhichScope();

#	print "#$_\n";
	my $functype, $funcname, $funcargs;

# split line to <NAME{$`}> ( <ARGS{$1}> ) parts.
	m/[ ]*\([ ]*(.*)[ ]*\)[ ]*([const]*)/;
	$funcname = $`;
	$funcargs = $1;
	$funcconst = $2;
	$funcname =~ s/[ ]+$//;
	$funcargs =~ s/[ ]+$//;

# handle func name.
	split(/[ ]+/,$funcname);
	$funcname = pop();
	$functype = join(' ', @_);

# handle func arg list.
#	split(/[ ]*,[ ]*/, $funcargs);
#	my @args = @_;
#		for (@args) {
#			if ( /[ ]*=[ ]*/ ) { $_ = $`; }		# for default value; ex. (short X = 100)
#			split(/[ ]+/);
#			pop();
#			$_ = join(' ',@_);
#		}
#	$funcargs = join(', ', @args);

	if ($functype) {
		$functype = $functype . " "
	}
	if ($funcconst) {
		$funcconst = " " . $funcconst
	}
	$_ = $functype . $funcname . "(" . $funcargs . ")" . $funcconst;
	ValidateDocbook();
#	print "FUNC = TYPE($funcname)+NAME($funcname)+ARGS($funcargs)\n";
#		print $funcname . "\t" x CalcTabs($funcname,5) . $functype . "(" . $funcargs . ")";
#		print "\n";
}

################
# ValidateDocbook
################

sub ValidateDocbook {

	open(IN2, $gDocbookpath) || die "Can't open '$gDocbookpath'";
	my $line = "";
	my $last1 = "";
	my $last2 = "";
	my $input = $_ ;
	my $pattern = "<title>" . $_ . "</title>" ;
	my $found = 0;
	while (<IN2>) {
		chomp;
		s/^[\t ]+//;
		s/[\t ]+$//;
		$line = $last1 . $last2 . " " . $_ ;
		if ( index($line, $pattern) >= 0 ) {
			$found = 1;
			break;
		}
		$last1 = $last2;
		$last2 = $_;
	}
	if ( $found == 0 ) {
		print $input . "\n" ;
	}
	close IN2;
}


################
# PrintVariable
################

sub PrintVariable {

# print a scope sign.
	print WhichScope();

#	print "#$_\n";
	my $vartype;
	my $varname = $_;
	my $varargs = "";

# split line to <NAME{$`}> [ <ARGS{$1}> ] parts.
	if ( m/[ ]*\[[ ]*(.*)[ ]*\][ ]*/ ) {
		$varname = $`;
		$varargs = $1;
		$varname =~ s/[ ]+$//;
		$varargs =~ s/[ ]+$//;
	}

	split(/[ ]+/,$varname);
	my $vartype = join(' ', @_[0..($#_ - 1)]);
	my $varname = $_[$#_];

#	print variable definition
	print $vartype . "\t"  x CalcTabs($vartype,5). $varname;
	if ( $varargs ne "" ) { print "[" . $varargs . "]"; }

# print a comment.
	if ( $#gComment >= 0 ) {
		print "\t"  x CalcTabs($varname,4);
		print join("\t",@gComment);
	}

	print "\n";
}


################
# WhichScope()
################

sub WhichScope {
#	my ($textline) = @_;
	my $key;
	my $str;

	SWITCH: {
		( s/^virtual[ ]*// ) && do { $key = "v"; last SWITCH; };
		( s/^static[ ]*// ) && do { $key = "s"; last SWITCH; };
		do { $key = "."; last SWITCH; }
	};

	SWITCH: {
		($gScope =~ /^public/ ) && do { $str = "=" . $key . "="; last SWITCH; };
		($gScope =~ /^protected/ ) && do { $str = ">" . $key . ">"; last SWITCH; };
		($gScope =~ /^private/ ) && do { $str = "(" . $key . ")"; last SWITCH; };
		do { $str = " " . $key . " "; last SWITCH; };
	};

	return $str . "\t";
}


################
# StringWidth
################

sub StringWidth {
	my ($str) = @_;

# width table of 10-point Geneva font
	my %width_table = (
	' '=> 3, '!'=> 4, '"'=> 5, '#'=> 9, '\$'=> 7, '%'=>10, '&'=> 8, '\''=> 3, 
	'('=> 6, ')'=> 6, '*'=> 7, '+'=> 8, ','=> 4, '-'=> 7, '.'=> 3, '/'=> 6, 
	'0'=> 7, '1'=> 7, '2'=> 7, '3'=> 7, '4'=> 7, '5'=> 7, '6'=> 7, '7'=> 7, 
	'8'=> 7, '9'=> 7, ':'=> 3, ';'=> 4, '<'=> 5, '='=> 7, '>'=> 5, '?'=> 7, 
	'@'=> 9, 'A'=> 6, 'B'=> 7, 'C'=> 6, 'D'=> 7, 'E'=> 6, 'F'=> 6, 'G'=> 6, 
	'H'=> 7, 'I'=> 4, 'J'=> 6, 'K'=> 7, 'L'=> 6, 'M'=> 9, 'N'=> 7, 'O'=> 6, 
	'P'=> 7, 'Q'=> 6, 'R'=> 7, 'S'=> 6, 'T'=> 6, 'U'=> 7, 'V'=> 6, 'W'=>10, 
	'X'=> 6, 'Y'=> 6, 'Z'=> 6, '['=> 5, '\\'=> 5, ']'=> 5, '^'=> 5, '_'=> 7, 
	'`'=> 4, 'a'=> 5, 'b'=> 6, 'c'=> 5, 'd'=> 5, 'e'=> 5, 'f'=> 4, 'g'=> 5, 
	'h'=> 6, 'i'=> 4, 'j'=> 5, 'k'=> 6, 'l'=> 4, 'm'=> 9, 'n'=> 6, 'o'=> 5, 
	'p'=> 6, 'q'=> 5, 'r'=> 6, 's'=> 5, 't'=> 4, 'u'=> 6, 'v'=> 6, 'w'=> 8, 
	'x'=> 6, 'y'=> 6, 'z'=> 5, '{'=> 5, '|'=> 4, '}'=> 5, '~'=> 7
);

#	my $tab_width = 4 * $width_table{'0'};

	my $sum = 0;
	my $c;

	foreach $c ( split(//,$str) ) {
		$sum += $width_table{$c};
#	print "'$c' =>$width_table{$c}/$sum\n";
	}

	return $sum;
}


################
# CalcTabs
################

sub CalcTabs {
	my ($str,$tabn) = @_;
	my $tab_width = 4 * StringWidth('0');
	my $tab_position = $tabn * $tab_width;
	my $skip_dots = $tab_position - StringWidth($str);
	my $more_tabs = 1;

	if ( $skip_dots > 0 ) {
		$more_tabs += ($skip_dots - 1) / $tab_width;
	}

#	print "["; print "$more_tabs:$skip_dots"; print "]";

	return $more_tabs;
}


################
# ExitToShell()
################
# Macintosh specific routine

sub ExitToShell {

# if the runtime environment is a Macintosh.
	&MacPerl'Quit(1) if $gMAC;

# else then it is a Unix.
	exit(0);
}


################
# GetFileInfo()
################
# Macintosh specific routine
# Returns a list ($creator,$type)

sub GetFileInfo {
	my ($filepath) = @_;

# if the runtime environment is a Macintosh.
	return &MacPerl'GetFileInfo($filepath) if $gMAC;

# else then it is a Unix and if the file is a text file.
	return ('????', 'TEXT') if -T $filepath;

# otherwise returns nil.
	return ('????', '????');
}

### end of script ###
