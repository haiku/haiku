/* test - BFS B+Tree torture test
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include "Volume.h"
#include "Inode.h"
#include "BPlusTree.h"

#include <List.h>

#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


#define DEFAULT_ITERATIONS	10
#define DEFAULT_NUM_KEYS	100
#define DEFAULT_KEY_TYPE	S_STR_INDEX

#define MIN_STRING 3
#define MAX_STRING 256

struct key {
	void	*data;
	uint32	length;
	int32	in;
	int32	count;
	off_t	value;
};

key *gKeys;
int32 gNum = DEFAULT_NUM_KEYS;
int32 gType = DEFAULT_KEY_TYPE;
int32 gTreeCount = 0;
bool gVerbose, gExcessive;
int32 gIterations = DEFAULT_ITERATIONS;
int32 gHard = 1;
Volume *gVolume;
int32 gSeed = 42;

// from cache.cpp (yes, we are that mean)
extern BList gBlocks;


// prototypes
void bailOut();
void bailOutWithKey(void *key, uint16 length);
void dumpTree();
void dumpKey(void *key, int32 length);
void dumpKeys();


void
dumpTree()
{
	puts("\n*** Tree-Dump:\n");

	bplustree_header *header = (bplustree_header *)gBlocks.ItemAt(0);
	dump_bplustree_header(header);

	for (int32 i = 1;i < gBlocks.CountItems();i++) {
		bplustree_node *node = (bplustree_node *)gBlocks.ItemAt(i);
		printf("\n--- %s node at %ld --------------------------------------\n",
				node->overflow_link == BPLUSTREE_NULL ? "leaf" : "index",
				i * BPLUSTREE_NODE_SIZE);
		dump_bplustree_node(node,header,gVolume);
	}
}


void
bailOut()
{
	if (gVerbose) {
		// dump the tree
		dumpTree();
	}

	// in any case, write the tree back to disk
	shutdown_cache(gVolume->Device(),gVolume->BlockSize());
	exit(-1);
}


void
bailOutWithKey(void *key, uint16 length)
{
	dumpKey(key, length);
	putchar('\n');
	bailOut();
}


void
dumpKey(void *key, int32 length)
{
	switch (gType) {
		case S_STR_INDEX:
			printf("\"%s\" (%ld bytes)", (char *)key, length);
			break;
		case S_INT_INDEX:
			printf("%ld", *(int32 *)key);
			break;
		case S_UINT_INDEX:
			printf("%lu", *(uint32 *)key);
			break;
		case S_LONG_LONG_INDEX:
			printf("%Ld", *(int64 *)key);
			break;
		case S_ULONG_LONG_INDEX:
			printf("%Lu", *(uint64 *)key);
			break;
		case S_FLOAT_INDEX:
			printf("%g", *(float *)key);
			break;
		case S_DOUBLE_INDEX:
			printf("%g", *(double *)key);
			break;
	}
	if ((gType == S_INT_INDEX || gType == S_UINT_INDEX || gType == S_FLOAT_INDEX)
		&& length != 4)
		printf(" (wrong length %ld)",length);
	else if ((gType == S_LONG_LONG_INDEX || gType == S_ULONG_LONG_INDEX || gType == S_DOUBLE_INDEX)
		&& length != 8)
		printf(" (wrong length %ld)",length);
}


void
dumpKeys()
{
	char *type;
	switch (gType) {
		case S_STR_INDEX:
			type = "string";
			break;
		case S_INT_INDEX:
			type = "int32";
			break;
		case S_UINT_INDEX:
			type = "uint32";
			break;
		case S_LONG_LONG_INDEX:
			type = "int64";
			break;
		case S_ULONG_LONG_INDEX:
			type = "uint64";
			break;
		case S_FLOAT_INDEX:
			type = "float";
			break;
		case S_DOUBLE_INDEX:
			type = "double";
			break;
	}
	printf("Dumping %ld keys of type %s\n",gNum,type);
	
	for (int32 i = 0;i < gNum;i++) {
		printf("% 8ld. (%3ld) key = ",i,gKeys[i].in);
		dumpKey(gKeys[i].data,gKeys[i].length);
		putchar('\n');
	}
}


//	#pragma mark -
//
//	Functions to generate the keys in every available type
//


void
generateName(int32 i,char *name,int32 *_length)
{
	// We're using the index position as a hint for the length
	// of the string - this way, it's much less expansive to
	// test for string uniqueness.
	// We don't want to sort the strings to have more realistic
	// access patterns to the tree (only true for the strings test).
	int32 length = i % (MAX_STRING - MIN_STRING) + MIN_STRING;
	for (int32 i = 0;i < length;i++) {
		int32 c = int32(52.0 * rand() / RAND_MAX);
		if (c >= 26)
			name[i] = 'A' + c - 26;
		else
			name[i] = 'a' + c;
	}
	name[length] = 0;
	*_length = length;
}


void
fillBuffer(void *buffer,int32 start)
{
	for (int32 i = 0;i < gNum;i++) {
		switch (gType) {
			case S_INT_INDEX:
			{
				int32 *array = (int32 *)buffer;
				array[i] = start + i;
				break;
			}
			case S_UINT_INDEX:
			{
				uint32 *array = (uint32 *)buffer;
				array[i] = start + i;
				break;
			}
			case S_LONG_LONG_INDEX:
			{
				int64 *array = (int64 *)buffer;
				array[i] = start + i;
				break;
			}
			case S_ULONG_LONG_INDEX:
			{
				uint64 *array = (uint64 *)buffer;
				array[i] = start + i;
				break;
			}
			case S_FLOAT_INDEX:
			{
				float *array = (float *)buffer;
				array[i] = start + i * 1.0001;
				break;
			}
			case S_DOUBLE_INDEX:
			{
				double *array = (double *)buffer;
				array[i] = start + i * 1.0001;
				break;
			}
		}
		gKeys[i].value = i;
	}
}


bool
findKey(void *key, int32 length, int32 maxIndex)
{
	for (int32 i = length;i < maxIndex;i += MAX_STRING - MIN_STRING) {
		if (length == (int32)gKeys[i].length
			&& !memcpy(key, gKeys[i].data, length))
			return true;
	}
	return false;
}


status_t
createKeys()
{
	gKeys = (key *)malloc(gNum * sizeof(key));
	if (gKeys == NULL)
		return B_NO_MEMORY;

	if (gType == S_STR_INDEX) {
		for (int32 i = 0;i < gNum;i++) {
			char name[B_FILE_NAME_LENGTH];
			int32 length,tries = 0;
			bool last;

			// create unique keys!
			do {
				generateName(i,name,&length);
			} while ((last = findKey(name,length,i)) && tries++ < 100);
			
			if (last) {
				printf("Couldn't create unique key list!\n");
				dumpKeys();
				bailOut();
			}

			gKeys[i].data = malloc(length + 1);
			memcpy(gKeys[i].data,name,length + 1);
			gKeys[i].length = length;
			gKeys[i].in = 0;
			gKeys[i].count = 0;
			gKeys[i].value = i;
		}
	} else {
		int32 length;
		int32 start = 0;
		switch (gType) {
			case S_FLOAT_INDEX:
			case S_INT_INDEX:
				start = -gNum / 2;
			case S_UINT_INDEX:
				length = 4;
				break;
			case S_DOUBLE_INDEX:
			case S_LONG_LONG_INDEX:
				start = -gNum / 2;
			case S_ULONG_LONG_INDEX:
				length = 8;
				break;
			default:
				return B_BAD_VALUE;
		}
		uint8 *buffer = (uint8 *)malloc(length * gNum);
		if (buffer == NULL)
			return B_NO_MEMORY;

		for (int32 i = 0;i < gNum;i++) {
			gKeys[i].data = (void *)(buffer + i * length);
			gKeys[i].length = length;
			gKeys[i].in = 0;
			gKeys[i].count = 0;
		}
		fillBuffer(buffer,start);
	}
	return B_OK;
}


//	#pragma mark -
//
//	Tree validity checker
//


void
checkTreeContents(BPlusTree *tree)
{
	// reset counter
	for (int32 i = 0;i < gNum;i++)
		gKeys[i].count = 0;

	TreeIterator iterator(tree);
	char key[B_FILE_NAME_LENGTH];
	uint16 length,duplicate;
	off_t value;
	status_t status;
	while ((status = iterator.GetNextEntry(key,&length,B_FILE_NAME_LENGTH,&value,&duplicate)) == B_OK) {
		if (value < 0 || value >= gNum) {
			iterator.Dump();
			printf("\ninvalid value %Ld in tree: ",value);
			bailOutWithKey(key,length);
		}
		if (gKeys[value].value != value) {
			iterator.Dump();
			printf("\nkey pointing to the wrong value %Ld (should be %Ld)\n",value,gKeys[value].value);
			bailOutWithKey(key,length);
		}
		if (length != gKeys[value].length
			|| memcmp(key,gKeys[value].data,length)) {
			iterator.Dump();
			printf("\nkeys don't match (key index = %Ld, %ld times in tree, %ld. occassion):\n\tfound: ",value,gKeys[value].in,gKeys[value].count + 1);
			dumpKey(key,length);
			printf("\n\texpected: ");
			dumpKey(gKeys[value].data,gKeys[value].length);
			putchar('\n');
			bailOut();
		}

		gKeys[value].count++;
	}
	if (status != B_ENTRY_NOT_FOUND) {
		printf("TreeIterator::GetNext() returned: %s\n",strerror(status));
		iterator.Dump();
		bailOut();
	}

	for (int32 i = 0;i < gNum;i++) {
		if (gKeys[i].in != gKeys[i].count) {
			printf("Key ");
			dumpKey(gKeys[i].data,gKeys[i].length);
			printf(" found only %ld from %ld\n",gKeys[i].count,gKeys[i].in);
		}
	}
}


void
checkTreeIntegrity(BPlusTree *tree)
{
	// simple test, just seeks down to every key - if it couldn't
	// be found, something must be wrong

	TreeIterator iterator(tree);
	for (int32 i = 0;i < gNum;i++) {
		if (gKeys[i].in == 0)
			continue;
		
		status_t status = iterator.Find((uint8 *)gKeys[i].data,gKeys[i].length);
		if (status != B_OK) {
			printf("TreeIterator::Find() returned: %s\n",strerror(status));
			bailOutWithKey(gKeys[i].data,gKeys[i].length);
		}
	}
}


void
checkTree(BPlusTree *tree)
{
	if (!gExcessive)
		printf("* Check tree...\n");

	checkTreeContents(tree);
	checkTreeIntegrity(tree);
}


//	#pragma mark -
//
//	The tree "torture" functions
//


void
addAllKeys(Transaction *transaction, BPlusTree *tree)
{
	printf("*** Adding all keys to the tree...\n");
	for (int32 i = 0;i < gNum;i++) {
		status_t status = tree->Insert(transaction,(uint8 *)gKeys[i].data,gKeys[i].length,gKeys[i].value);
		if (status < B_OK) {
			printf("BPlusTree::Insert() returned: %s\n",strerror(status));
			printf("key: ");
			dumpKey(gKeys[i].data,gKeys[i].length);
			putchar('\n');
		}
		else {
			gKeys[i].in++;
			gTreeCount++;
		}
	}
	checkTree(tree);
}


void
removeAllKeys(Transaction *transaction, BPlusTree *tree)
{
	printf("*** Removing all keys from the tree...\n");
	for (int32 i = 0;i < gNum;i++) {
		while (gKeys[i].in > 0) {
			status_t status = tree->Remove(transaction, (uint8 *)gKeys[i].data,
				gKeys[i].length, gKeys[i].value);
			if (status < B_OK) {
				printf("BPlusTree::Remove() returned: %s\n", strerror(status));
				printf("key: ");
				dumpKey(gKeys[i].data, gKeys[i].length);
				putchar('\n');
			}
			else {
				gKeys[i].in--;
				gTreeCount--;
			}
		}
	}
	checkTree(tree);
	
}


void
duplicateTest(Transaction *transaction,BPlusTree *tree)
{
	int32 index = int32(1.0 * gNum * rand() / RAND_MAX);
	if (index == gNum)
		index = gNum - 1;

	printf("*** Duplicate test with key ");
	dumpKey(gKeys[index].data,gKeys[index].length);
	puts("...");

	status_t status;

	int32 insertTotal = 0;
	for (int32 i = 0;i < 8;i++) {
		int32 insertCount = int32(1000.0 * rand() / RAND_MAX);
		if (gVerbose)
			printf("* insert %ld to %ld old entries...\n",insertCount,insertTotal + gKeys[index].in);

		for (int32 j = 0;j < insertCount;j++) {
			status = tree->Insert(transaction,(uint8 *)gKeys[index].data,gKeys[index].length,insertTotal);
			if (status < B_OK) {
				printf("BPlusTree::Insert() returned: %s\n",strerror(status));
				bailOutWithKey(gKeys[index].data,gKeys[index].length);
			}
			insertTotal++;
			gTreeCount++;
	
			if (gExcessive)
				checkTree(tree);
		}

		int32 count;
		if (i < 7) {
			count = int32(1000.0 * rand() / RAND_MAX);
			if (count > insertTotal)
				count = insertTotal;
		} else
			count = insertTotal;

		if (gVerbose)
			printf("* remove %ld from %ld entries...\n",count,insertTotal + gKeys[index].in);

		for (int32 j = 0;j < count;j++) {
			status_t status = tree->Remove(transaction,(uint8 *)gKeys[index].data,gKeys[index].length,insertTotal - 1);
			if (status < B_OK) {
				printf("BPlusTree::Remove() returned: %s\n",strerror(status));
				bailOutWithKey(gKeys[index].data,gKeys[index].length);
			}
			insertTotal--;
			gTreeCount--;

			if (gExcessive)
				checkTree(tree);
		}
	}

	if (!gExcessive)
		checkTree(tree);
}


void
addRandomSet(Transaction *transaction,BPlusTree *tree,int32 num)
{
	printf("*** Add random set to tree (%ld to %ld old entries)...\n",num,gTreeCount);

	for (int32 i = 0;i < num;i++) {
		int32 index = int32(1.0 * gNum * rand() / RAND_MAX);
		if (index == gNum)
			index = gNum - 1;

		if (gVerbose)
			printf("adding key %ld (%ld times in the tree)\n",index,gKeys[index].in);

		status_t status = tree->Insert(transaction,(uint8 *)gKeys[index].data,gKeys[index].length,gKeys[index].value);
		if (status < B_OK) {
			printf("BPlusTree::Insert() returned: %s\n",strerror(status));
			bailOutWithKey(gKeys[index].data,gKeys[index].length);
		}
		gKeys[index].in++;
		gTreeCount++;

		if (gExcessive)
			checkTree(tree);
	}
	if (!gExcessive)
		checkTree(tree);
}


void
removeRandomSet(Transaction *transaction,BPlusTree *tree,int32 num)
{
	printf("*** Remove random set from tree (%ld from %ld entries)...\n",num,gTreeCount);

	int32 tries = 500;

	for (int32 i = 0;i < num;i++) {
		if (gTreeCount < 1)
			break;

		int32 index = int32(1.0 * gNum * rand() / RAND_MAX);
		if (index == gNum)
			index = gNum - 1;

		if (gKeys[index].in == 0) {
			i--;
			if (tries-- < 0)
				break;
			continue;
		}

		if (gVerbose)
			printf("removing key %ld (%ld times in the tree)\n",index,gKeys[index].in);

		status_t status = tree->Remove(transaction,(uint8 *)gKeys[index].data,gKeys[index].length,gKeys[index].value);
		if (status < B_OK) {
			printf("BPlusTree::Remove() returned: %s\n",strerror(status));
			bailOutWithKey(gKeys[index].data,gKeys[index].length);
		}
		gKeys[index].in--;
		gTreeCount--;

		if (gExcessive)
			checkTree(tree);
	}
	if (!gExcessive)
		checkTree(tree);
}


//	#pragma mark -


void
usage(char *program)
{
	if (strrchr(program,'/'))
		program = strrchr(program,'/') + 1;
	fprintf(stderr,"usage: %s [-veh] [-t type] [-n keys] [-i iterations] [-h times] [-r seed]\n"
		"BFS B+Tree torture test\n"
		"\t-t\ttype is one of string, int32, uint32, int64, uint64, float,\n"
		"\t\tor double; defaults to string.\n"
		"\t-n\tkeys is the number of keys to be used,\n"
		"\t\tminimum is 1, defaults to %d.\n"
		"\t-i\titerations is the number of the test cycles, defaults to %d.\n"
		"\t-r\tthe seed for the random function, defaults to %ld.\n"
		"\t-h\tremoves the keys and start over again for x times.\n"
		"\t-e\texcessive validity tests: tree contents will be tested after every operation\n"
		"\t-v\tfor verbose output.\n",
		program, DEFAULT_NUM_KEYS, DEFAULT_ITERATIONS, gSeed);
	exit(0);
}


int
main(int argc,char **argv)
{
	char *program = argv[0];

	while (*++argv)
	{
		char *arg = *argv;
		if (*arg == '-')
		{
			if (arg[1] == '-')
				usage(program);

			while (*++arg && isalpha(*arg))
			{
				switch (*arg)
				{
					case 'v':
						gVerbose = true;
						break;
					case 'e':
						gExcessive = true;
						break;
					case 't':
						if (*++argv == NULL)
							usage(program);

						if (!strcmp(*argv,"string"))
							gType = S_STR_INDEX;
						else if (!strcmp(*argv,"int32")
							|| !strcmp(*argv,"int"))
							gType = S_INT_INDEX;
						else if (!strcmp(*argv,"uint32")
							|| !strcmp(*argv,"uint"))
							gType = S_UINT_INDEX;
						else if (!strcmp(*argv,"int64")
							|| !strcmp(*argv,"llong"))
							gType = S_LONG_LONG_INDEX;
						else if (!strcmp(*argv,"uint64")
							|| !strcmp(*argv,"ullong"))
							gType = S_ULONG_LONG_INDEX;
						else if (!strcmp(*argv,"float"))
							gType = S_FLOAT_INDEX;
						else if (!strcmp(*argv,"double"))
							gType = S_DOUBLE_INDEX;
						else
							usage(program);
						break;
					case 'n':
						if (*++argv == NULL || !isdigit(**argv))
							usage(program);
						
						gNum = atoi(*argv);
						if (gNum < 1)
							gNum = 1;
						break;
					case 'h':
						if (*++argv == NULL || !isdigit(**argv))
							usage(program);

						gHard = atoi(*argv);
						if (gHard < 1)
							gHard = 1;
						break;
					case 'i':
						if (*++argv == NULL || !isdigit(**argv))
							usage(program);

						gIterations = atoi(*argv);
						if (gIterations < 1)
							gIterations = 1;
						break;
					case 'r':
						if (*++argv == NULL || !isdigit(**argv))
							usage(program);
						
						gSeed = atoi(*argv);
						break;
				}
			}
		}
		else
			break;
	}

	// we do want to have reproducible random keys
	if (gVerbose)
		printf("Set seed to %ld\n",gSeed);
	srand(gSeed);
	
	Inode inode("tree.data",gType | S_ALLOW_DUPS);
	gVolume = inode.GetVolume();
	Transaction transaction(gVolume,0);

	init_cache(gVolume->Device(),gVolume->BlockSize());

	//
	// Create the tree, the keys, and add all keys to the tree initially
	//

	BPlusTree tree(&transaction,&inode);
	status_t status;
	if ((status = tree.InitCheck()) < B_OK) {
		fprintf(stderr,"creating tree failed: %s\n",strerror(status));
		bailOut();
	}
	printf("*** Creating %ld keys...\n",gNum);
	if ((status = createKeys()) < B_OK) {
		fprintf(stderr,"creating keys failed: %s\n",strerror(status));
		bailOut();
	}

	if (gVerbose)
		dumpKeys();

	for (int32 j = 0; j < gHard; j++ ) {
		addAllKeys(&transaction, &tree);

		//
		// Run the tests (they will exit the app, if an error occurs)
		//
	
		for (int32 i = 0;i < gIterations;i++) {
			printf("---------- Test iteration %ld ---------------------------------\n",i+1);
	
			addRandomSet(&transaction,&tree,int32(1.0 * gNum * rand() / RAND_MAX));
			removeRandomSet(&transaction,&tree,int32(1.0 * gNum * rand() / RAND_MAX));
			duplicateTest(&transaction,&tree);
		}
	
		removeAllKeys(&transaction, &tree);
	}

	// of course, we would have to free all our memory in a real application here...

	// write the cache back to the tree
	shutdown_cache(gVolume->Device(),gVolume->BlockSize());
	return 0;
}

