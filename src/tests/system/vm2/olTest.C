#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "olist.h"
#include "error.h"
#include "OS.h"

struct iltTest : public node
{
	iltTest(int i) {value=i;}
	int value;	
};

bool ilt(void *a,void *b)
{
	iltTest *a1=reinterpret_cast<iltTest *>(a);
	iltTest *b1=reinterpret_cast<iltTest *>(b);
	if (a1->value<b1->value)
		return true;
	return false;
}

int main(int argc,char **argv)
{

	// Test 1 - Try to add to foo without setting an isLessThan function
	try {
		orderedList foo;
		node tmp;
		foo.add(&tmp);
		}
	catch (const char *badness)
		{
		if (strcmp(badness,"Attempting to use an ordered list without setting up a 'toLessThan' function"))
			printf ("Failure on adding with no isLessThan, printf = %s\n", badness);
		printf ("Success on adding with no isLessThan, \n" );
		}
	catch(...)
		{
		printf ("Failure on adding with no isLessThan, unknown exception\n");
		}

	orderedList foo;
	foo.setIsLessThan(ilt);

	printf ("Add the first couple, in order (easy)\n");
	iltTest first(1);
	foo.add(&first);
	iltTest second(2);
	foo.add(&second);
	foo.dump();

	printf ("Add the next three, not in order (harder)\n");
	iltTest third(3);
	iltTest fourth(4);
	iltTest fifth(5);
	foo.add(&fourth);
	foo.add(&third);
	foo.add(&fifth);
	foo.dump();
	while (iltTest *n=(iltTest *)(foo.next()))
		printf ("Popped %d\n",n->value);

	orderedList bar;
	bar.setIsLessThan(ilt);
	while (iltTest *n=(iltTest *)(bar.next()))
		printf ("Popped %d\n",n->value);
	for (int a=0;a<100;a++)
		bar.add(new iltTest(a));
	while (iltTest *n=(iltTest *)(bar.next()))
		printf ("Popped %d\n",n->value);
	for (int a=1000;a>=0;a--)
		bar.add(new iltTest(a));
	while (iltTest *n=(iltTest *)(bar.next()))
		if (!(n->value%50)) printf ("Popped %d\n",n->value);

	printf ("Setting up simple case\n");
	foo.add(&first);
	foo.dump();
	printf ("Removing simple case\n");
	foo.remove(&first);
	foo.dump();

	printf ("Setting up middle case\n");
	foo.add(&first);
	foo.add(&second);
	foo.dump();
	printf ("Removing middle case\n");
	foo.remove(&second);
	foo.dump();
	foo.remove(&first);

	printf ("Setting up final case\n");
	foo.add(&first);
	foo.add(&second);
	foo.add(&third);
	foo.dump();
	printf ("Removing final case\n");
	foo.remove(&second);
	foo.dump();
	return 0;
}
