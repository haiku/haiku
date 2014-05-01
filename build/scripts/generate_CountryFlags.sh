#!/bin/sh

# Generate LocaleKit's CountryFlags.rdef
# from all flag rdefs in the current folder

destination=CountryFlags.rdef
nr=0

for file in *
do
	id=`echo "$file" | cut -b -2`
	name=`echo "${file%%.*}" | cut -b 4-`

	echo "// Flag data for $name" >> $destination
	echo "resource($nr,\"$id\") #'VICN' array {" >> $destination
	tail -n +3 "$file" >> $destination
	echo >> $destination

	nr=`expr $nr + 1`
	echo \ $nr, $name, $id... OK
done
