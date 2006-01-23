# Clean PCI header script
#
# Copyright 2006, Haiku.
# Distributed under the terms of the MIT License.
#
# Authors:
#		John Drinkwater, john@nextraweb.com
#
# Use with http://pcidatabase.com/pci_c_header.php
# run as (with sed 4) : sed -f pci_clean pci-input > pci-output

#s/[^:]\/\/.*$//							# strip comments
#/^ /d
#/^$/d
s/[^][A-Za-z0-9 "{}_,;=#!*()'.:+@/\t&\-]//g  # strip unknown characters
#s/[^[:alnum:][:punct:][:space:]]//g	  is this cleaner ?
s/&#[[:xdigit:]]*;//g					# remove unicode ..
/, ,/d									# IDs were in escaped unicode! we`ve already removed them, so delete
/"", ""/d								# for entries with no information, just remove
s/[ÃÂ©]//g								# remove UTF prefixes, and other junk
s/???//g								# replace filler with something cleaner
s/(?)//g								# remove “Whats this” uncertanty
s/0X\([[:xdigit:]]\)/0x\1/g				# some lines dont have clean xs
s/0\*\([[:xdigit:]]\)/0x\1/g			# some lines dont have clean xs
s/0x /0x/g								# clean redunant space
#/x[A-Fa-f0-9]*O[A-Fa-f0-9]*/y/O/0/		# Greedy. Todo: find a better replacement ?
/x[[:xdigit:]]*O[[:xdigit:]]*/ {		# cull stupid people
	s/O/0/								# Only does the first..
}
/0x[[:xdigit:]]\{5,8\}/d				# a weird ID, remove.  Todo: improve
s/0xIBM\([[:xdigit:]]\{4\}\)/0x\1/g		# stupid ID. Todo: delete { wait for pcidatabase }
										# clean ##(#(#)) IDs into 0x((0)0)##
s/^\([^"]*\)[^x"[:xdigit:]]\([[:xdigit:]]\{4\}\),/\1 0x\2,/g
s/^\([^"]*\)[^x"[:xdigit:]]\([[:xdigit:]]\{3\}\),/\1 0x0\2,/g
s/^\([^"]*\)[^x"[:xdigit:]]\([[:xdigit:]]\{2\}\),/\1 0x00\2,/g
										# clean 0x### IDs into 0x0###
s/0x\([[:xdigit:]]\{3\}\),/0x0\1,/g		

# mostly clean.. now we just strip the lines that dont obey!

										# look at device table..
/\t{ 0x[[:xdigit:]]\{4\}, [^"]/ {	
	# because devices classes shouldn't be looked at

	s/0x\([[:xdigit:]]\{2\}\),/0x00\1,/g	# make it all 4 width	
	/, 0x[A-Fa-f0-9]\{4\},/!d				# check device IDs are OK
}

