#ifndef __testing_is_delightful_h__
#define __testing_is_delightful_h__

#include <TestShell.h>
#include <string>

class UnitTesterShell : public BTestShell {
public:
	UnitTesterShell(const std::string &description = "", SyncObject *syncObject = 0);
protected:
//	static const std::string defaultLibDir;
	bool doR5Tests;
	virtual void PrintDescription(int argc, char *argv[]);
	virtual void PrintValidArguments();
	virtual bool ProcessArgument(std::string arg, int argc, char *argv[]);
	virtual void LoadDynamicSuites();
};

//extern UnitTesterShell shell;

#endif // __testing_is_delightful_h__
