#!/bin/bash

if [ $# -eq 2 ]
  then
	OLD=$1
	NEW=$2

	# We need a tab character as a field separator
	TAB=`echo -e "\t"`

	#Temporary storage
	TEMPFILE=`mktemp /tmp/catmerge.XXXXX`

	# Extract the list of keys to remove
	# Compare (diff) the keys only (cut) ; keep only 'removed' lines (grep -),
	# Ignore diff header and headlines from both files (tail), remove diff's
	# prepended stuff (cut)
	# Put the result in our tempfile.
	diff -u <(cut -f 1,2 $OLD) <(cut -f 1,2 $NEW) |grep ^-|\
		tail -n +3|cut -b2- > $TEMPFILE

	# Reuse the headline from the new file (including fingerprint). This gets
	# the language wrong, but it isn't actually used anywhere
	head -1 $NEW
	# Sort-merge old and new, inserting lines from NEW into OLD (sort);
	#	Exclude the headline from that (tail -n +2)
	# Then, filter out the removed strings (fgrep)
	sort -u -t"$TAB" -k 1,2 <(tail -n +2 $OLD) <(tail -n +2 $NEW)|\
		fgrep -v -f $TEMPFILE

	rm $TEMPFILE

  else
	echo "$0 OLD NEW"
	echo "merges OLD and NEW catalogs, such that all the keys in NEW that are"
	echo "not yet in OLD are added to it, and the one in OLD but not in NEW are"
	echo "removed. The fingerprint is also updated."
fi
