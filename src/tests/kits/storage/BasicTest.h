// BasicTest.h

#ifndef __sk_basic_test_h__
#define __sk_basic_test_h__

#include <SupportDefs.h>
#include <TestCase.h>
#include <TestShell.h>
#include <set>
#include <stdio.h>

class BasicTest : public BTestCase
{
public:
	BasicTest();

	// This function called before *each* test added in Suite()
	void setUp();
	
	// This function called after *each* test added in Suite()
	void tearDown();

	// helper functions

//	void nextSubTest();
//	void nextSubTestBlock();

	static void execCommand(const string &command);

	static void dumpStat(struct stat &st);

	static void createVolume(string imageFile, string mountPoint, int32 megs,
							 bool makeMountPoint = true);
	static void deleteVolume(string imageFile, string mountPoint,
							 bool deleteMountPoint = true);

protected:	
	int32 fSubTestNumber;
	int32 fAvailableFDs;
};


// Some other helpful stuff.

// == for struct stat
static
inline
bool
operator==(const struct stat &st1, const struct stat &st2)
{
	return (
		st1.st_dev == st2.st_dev
		&& st1.st_ino == st2.st_ino
		&& st1.st_mode == st2.st_mode
		&& st1.st_nlink == st2.st_nlink
		&& st1.st_uid == st2.st_uid
		&& st1.st_gid == st2.st_gid
		&& st1.st_size == st2.st_size
		&& st1.st_blksize == st2.st_blksize
		&& st1.st_atime == st2.st_atime
		&& st1.st_mtime == st2.st_mtime
		&& st1.st_ctime == st2.st_ctime
		&& st1.st_crtime == st2.st_crtime
	);
}

// first parameter is equal to the second or third
template<typename A, typename B, typename C>
static
inline
bool
equals(const A &a, const B &b, const C &c)
{
	return (a == b || a == c);
}

// A little helper class for tests. It works like a set of strings, that
// are marked tested or untested.
class TestSet {
public:
	typedef set<string> nameset;

public:
	TestSet()
	{
	}

	void add(string name)
	{
		if (fTestedNames.find(name) == fTestedNames.end())
			fUntestedNames.insert(name);
	}

	void remove(string name)
	{
		if (fUntestedNames.find(name) != fUntestedNames.end())
			fUntestedNames.erase(name);
		else if (fTestedNames.find(name) != fTestedNames.end())
			fTestedNames.erase(name);
	}

	void clear()
	{
		fUntestedNames.clear();
		fTestedNames.clear();
	}

	void rewind()
	{
		fUntestedNames.insert(fTestedNames.begin(), fTestedNames.end());
		fTestedNames.clear();
	}

	bool test(string name, bool dump = BTestShell::GlobalBeVerbose())
	{
		bool result = (fUntestedNames.find(name) != fUntestedNames.end());
		if (result) {
			fUntestedNames.erase(name);
			fTestedNames.insert(name);
		} else if (dump) {
			// dump untested
			printf("TestSet::test(`%s')\n", name.c_str());
			printf("untested:\n");
			for (nameset::iterator it = fUntestedNames.begin();
				 it != fUntestedNames.end();
				 ++it) {
				printf("  `%s'\n", it->c_str());
			}
		}
		return result;
	}

	bool testDone()
	{
		return (fUntestedNames.empty());
	}

private:
	nameset	fUntestedNames;
	nameset	fTestedNames;
};


#endif	// __sk_basic_test_h__
