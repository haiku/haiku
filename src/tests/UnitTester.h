#ifndef __testing_is_delightful_h__
#define __testing_is_delightful_h__

#include <TestShell.h>

class UnitTesterShell : public BTestShell {
public:
	UnitTesterShell(const std::string &description = "", SyncObject *syncObject = 0);
	virtual void PrintDescription(int argc, char *argv[]);
};

//extern UnitTesterShell shell;

#endif // __testing_is_delightful_h__
