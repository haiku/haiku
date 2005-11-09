#ifndef __unit_tester_helper_h__
#define __unit_tester_helper_h__

#include <TestShell.h>
#include <string>

class UnitTesterShell : public BTestShell {
public:
	UnitTesterShell(const string &description = "", SyncObject *syncObject = 0);
protected:
	virtual void PrintDescription(int argc, char *argv[]);
	virtual void PrintValidArguments();
	virtual void LoadDynamicSuites();
};

//extern UnitTesterShell shell;

#endif // __unit_tester_helper_h__
