#ifndef _beos_test_shell_h_
#define _beos_test_shell_h_

#include <CppUnitShell.h>

class TestShell : public CppUnitShell {
public:
	TestShell(const std::string &description = "", SyncObject *syncObject = 0);
	~TestShell();
	int Run(int argc, char *argv[]);
//	virtual void PrintDescription(int argc, char *argv[]);
	const char* TestDir() const;
private:
	BPath *fTestDir;
};

#endif // _beos_test_shell_h_