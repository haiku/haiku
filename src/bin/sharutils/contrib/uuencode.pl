# uuencode in Perl - non tested.
# Copyright (C) 1995 Free Software Foundation, Inc.
# François Pinard <pinard@iro.umontreal.ca>, 1995.

print "begin 644 $ARGV[0]\n";
print pack ("u", $bloc) while read (STDIN, $bloc, 45);
print "`\n";
print "end\n";

exit 0;
