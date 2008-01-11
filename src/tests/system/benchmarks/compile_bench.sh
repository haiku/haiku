#!/bin/sh

testDir=/tmp/compile_bench
rm -rf $testDir
mkdir -p $testDir
cd $testDir

cat << EOF > hello_world.cpp
#include <stdio.h>

int
main()
{
	printf("Hello world!\n");
	return 0;
}

EOF

compile_all()
{
	for f in $(seq 100); do
		echo -n .
		g++ -o $f ${f}.cpp
	done
}

for f in $(seq 100); do
	cp hello_world.cpp ${f}.cpp
done

time compile_all

rm -rf $testDir
