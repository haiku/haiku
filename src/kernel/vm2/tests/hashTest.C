#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "hashTable.h"
#include "OS.h"
#include "vmInterface.h"

vmInterface vm(30);

struct hashTest : public node {
	hashTest(int i) {value=i;}
	int value;	
};

ulong hash(node &a) {
	hashTest &a1=reinterpret_cast<hashTest &>(a);
	return a1.value;
}

bool isEqual (node &a,node &b) {
	hashTest &a1=reinterpret_cast<hashTest &>(a);
	hashTest &b1=reinterpret_cast<hashTest &>(b);
	return (a1.value==b1.value);
}

int main(int argc,char **argv) {
	// Test 1 - Try to add to foo without setting an isLessThan function
	try {
		hashTable foo(40);
		node tmp;
		foo.add(&tmp);
		}
	catch (const char *badness)
		{
		if (strcmp(badness,"Attempting to use a hash table without setting up a 'hash' function"))
			error ("Failure on adding with no hash, error = %s\n", badness);
		error ("Success on adding with no hash, \n" );
		}
	catch(...)
		{
		error ("Failure on adding with no hash, unknown exception\n");
		}

	hashTable *moo;
	moo=new hashTable(20);  
	hashTable &foo=*moo;

	foo.setHash(hash);
	foo.setIsEqual(isEqual);

	error ("Add the first couple, in order (easy)\n");
	hashTest first(1);
	foo.add(&first);
	hashTest second(2);
	foo.add(&second);
	foo.dump();

	error ("Add the next three, not in order (harder)\n");
	hashTest third(3);
	hashTest fourth(4);
	hashTest fifth(5);
	foo.add(&fourth);
	foo.add(&third);
	foo.add(&fifth);
	foo.dump();

	error ("Create bar, populated with 1000 values");
	hashTable bar(200);
	bar.setHash(hash);
	bar.setIsEqual(isEqual);
	for (int a=0;a<100;a++)
		bar.add(new hashTest(a));
	for (int a=999;a>=100;a--)
		bar.add(new hashTest(a));
	bar.dump();

	error ("Removing simple case: remove 1,leave 2,3,4,5 \n");
	foo.remove(&first);
	foo.dump();
	error ("Removing simple case: remove 2,leave 3,4,5 \n");
	foo.remove(&second);
	foo.dump();

	error ("Setting up middle case (reading 1 and 2)\n");
	foo.add(&first);
	foo.add(&second);
	foo.dump();
	error ("Removing middle case (removing 2)\n");
	foo.remove(&second);
	foo.dump();
	error ("Removing middle case (removing 1)\n");
	foo.remove(&first);
	foo.dump();

	error ("Finding 1\n");
	hashTest findOne(1);
	hashTest *found=(hashTest *)bar.find(&findOne);
	error ("found value = %d\n",found->value);

	error ("Finding 77\n");
	hashTest findSeventySeven(77);
	hashTest *found77=(hashTest *)bar.find(&findSeventySeven);
	error ("found value = %d\n",found77->value);

	error ("ensuringSane on smaller (foo) = %s\n",((foo.ensureSane())?"OK":"BAD!"));
	error ("ensuringSane on larger (bar) = %s\n",((bar.ensureSane())?"OK":"BAD!"));

	
	int count=0;
	for (hashIterate hi(bar);node *next=hi.get();) {
		count++;
		if (next==NULL)
			error ("Found a NULL at %d\n",count);
		}
	if (count==1000)
		error ("found 1000, as expected!\n");
	else
		error ("did NOT find 1000, as expected, found %d!\n",count);
	delete moo;
	return 0;
}
