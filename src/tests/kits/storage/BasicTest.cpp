// BasicTest.cpp

#include <stdio.h>
#include <unistd.h>
#include <string>

#include "BasicTest.h"

// count_available_fds
#include <set>
static
int32
count_available_fds()
{
	set<int> fds;
	int fd;
	while ((fd = dup(1)) != -1)
		fds.insert(fd);
	for (set<int>::iterator it = fds.begin(); it != fds.end(); it++)
		close(*it);
	return fds.size();
}

// constructor
BasicTest::BasicTest()
		 : BTestCase(),
		   fSubTestNumber(0),
		   fAvailableFDs(0)
{
}

// setUp
void
BasicTest::setUp()
{
	BTestCase::setUp();
	fAvailableFDs = count_available_fds();
	SaveCWD();
	fSubTestNumber = 0;
}

// tearDown
void
BasicTest::tearDown()
{
	RestoreCWD();
	int32 availableFDs = count_available_fds();
	if (availableFDs != fAvailableFDs) {
		printf("WARNING: Number of available file descriptors has changed "
			   "during test: %ld -> %ld\n", fAvailableFDs, availableFDs);
		fAvailableFDs = availableFDs;
	}
	BTestCase::tearDown();
}

// execCommand
//
// Calls system() with the supplied string.
void
BasicTest::execCommand(const string &cmdLine)
{
	system(cmdLine.c_str());
}

// dumpStat
void
BasicTest::dumpStat(struct stat &st)
{
	printf("stat:\n");
	printf("  st_dev    : %lx\n", st.st_dev);
	printf("  st_ino    : %Lx\n", st.st_ino);
	printf("  st_mode   : %x\n", st.st_mode);
	printf("  st_nlink  : %x\n", st.st_nlink);
	printf("  st_uid    : %x\n", st.st_uid);
	printf("  st_gid    : %x\n", st.st_gid);
	printf("  st_size   : %Ld\n", st.st_size);
	printf("  st_blksize: %ld\n", st.st_blksize);
	printf("  st_atime  : %lx\n", st.st_atime);
	printf("  st_mtime  : %lx\n", st.st_mtime);
	printf("  st_ctime  : %lx\n", st.st_ctime);
	printf("  st_crtime : %lx\n", st.st_crtime);
}

// createVolume
void
BasicTest::createVolume(string imageFile, string mountPoint, int32 megs,
						bool makeMountPoint)
{
	char megsString[16];
	sprintf(megsString, "%ld", megs);
	execCommand(string("dd if=/dev/zero of=") + imageFile
					+ " bs=1M count=" + megsString
					+ " &> /dev/null"
				+ " ; mkbfs " + imageFile
					+ " > /dev/null"
				+ " ; sync"
				+ (makeMountPoint ? " ; mkdir " + mountPoint : "")
				+ " ; mount " + imageFile + " " + mountPoint);
}

// deleteVolume
void
BasicTest::deleteVolume(string imageFile, string mountPoint,
						bool deleteMountPoint)
{
	execCommand(string("sync")
				+ " ; unmount " + mountPoint
				+ (deleteMountPoint ? " ; rmdir " + mountPoint : "")
				+ " ; rm " + imageFile);
}

