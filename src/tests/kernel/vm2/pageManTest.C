#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "pageManager.h"
#include "OS.h"

enum testType {clean,unused,inuse};

pageManager pm;
char *testName;

bool testCount (int expected, testType type)
{
	int found;
	switch (type) {
		case clean:
			if (found=pm.getCleanCount()!=expected) {
				error ("%s, invalid cleanCount of %d, expected: %d\n",testName,found,expected);
				return false;
				}
			break;
		case unused:
			if (found=pm.getUnusedCount()!=expected) {
				error ("%s, invalid unusedCount of %d, expected: %d\n",testName,found,expected);
				return false;
				}
			break;
		case inuse:
			if (found=pm.getInUseCount()!=expected) {
				error ("%s, invalid InUseCount of %d, expected: %d\n",testName,found,expected);
				return false;
				}
			break;
	}
	return true;
}

int main(int argc,char **argv)
{
	testName="Setup";

	pm.setup(malloc(PAGE_SIZE*100),100);

	if (!(testCount(99,unused) || testCount (0,clean) || testCount (0,inuse))) {
		pm.dump();
		exit(1);
		}

	testName="Get One";
	page *gotten = pm.getPage();
	if (!(testCount(98,unused) || testCount (0,clean) || testCount (1,inuse))) {
		pm.dump();
		exit(1);
		}

	testName="Put One";
	pm.freePage(gotten);
	if (!(testCount(99,unused) || testCount (0,clean) || testCount (0,inuse))) {
		pm.dump();
		exit(1);
		}

	testName="Clean One";
	pm.cleanOnePage();
	if (!(testCount(98,unused) || testCount (1,clean) || testCount (0,inuse))) {
		pm.dump();
		exit(1);
		}
	
	testName="Get One Cleaned";
	gotten = pm.getPage();
	if (!(testCount(98,unused) || testCount (0,clean) || testCount (1,inuse))) {
		pm.dump();
		exit(1);
		}

	testName="Put One Cleaned";
	pm.freePage(gotten);
	if (!(testCount(99,unused) || testCount (0,clean) || testCount (0,inuse))) {
		pm.dump();
		exit(1);
		}

	testName="Get them all";
	page *pages[100];
	for (int i=0;i<99;i++)
		pages[i]=pm.getPage();
	if (!(testCount(0,unused) || testCount (0,clean) || testCount (99,inuse))) {
		pm.dump();
		exit(1);
		}

	testName="Free them all";
	for (int i=0;i<99;i++)
		pm.freePage(pages[i]);
	if (!(testCount(99,unused) || testCount (0,clean) || testCount (0,inuse))) {
		pm.dump();
		exit(1);
		}
	exit(0);
}
