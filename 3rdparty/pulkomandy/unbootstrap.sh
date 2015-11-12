#!/bin/sh
# Convert _bootstrap packages to non-bootstrap version.
# THIS IS A HACK. The _bootstrap packages are cross-compiled on a pseudo-haiku
# environment running on a different host system. Expect binary
# incompatibilities when trying to use them. However, the development files
# should still be valid, allowing us to build a non-bootstrap Haiku against
# these package before we can get the bootstrap image to run reliably enough
# to actually bootstrap the packages.

mkdir -p work
cd work

for package in ../packages/*.hpkg
do
	echo --- Processing $package ---
	echo Cleaning work directory...
	rm -r *
	echo Extracting package...
	package extract $package
	echo Converting .PackageInfo...
	sed -i .PackageInfo -e s/_bootstrap//g
	name=`basename $package|sed -e s/_bootstrap//g`
	echo Regenerating package...
	package create ../$name
done
