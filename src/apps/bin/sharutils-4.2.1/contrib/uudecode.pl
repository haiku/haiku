# uuencode in Perl.
# Copyright (C) 1995 Free Software Foundation, Inc.
# François Pinard <pinard@iro.umontreal.ca>, 1995.

# `perl uudecode.pl FILES' will decode all uuencoded files found in
# all input FILES, stripping headers and other non uuencoded data.

while (<>)
{
    if (/^begin [0-7][0-7][0-7] ([^\n ]+)$/)
    {
	open (OUTPUT, ">$1") || die "Cannot create $1\n";
	binmode OUTPUT;
	while (<>)
	{
	    last if /^end$/;
	    $block = unpack ("u", $_);
	    print OUTPUT $block;
	}
	close OUTPUT;
    }
}

exit 0;
