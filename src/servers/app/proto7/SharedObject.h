#ifndef SHAREDOBJ_H_
#define SHAREDOBJ_H_

/*
	This class is used as a base class for other objects which must be shared.
	By using this, objects which depend on its existence can be somewhat sure
	that the shared object will not be deleted until nothing depends on it.
*/
#include <SupportDefs.h>

class SharedObject
{
public:
	SharedObject(void) { dependents=0; }
	virtual ~SharedObject(void){}
	virtual void AddDependent(void) { dependents++; }
	virtual void RemoveDependent(void) { dependents--; }
	virtual bool HasDependents(void) { return (dependents>0)?true:false; }
private:
	uint32 dependents;
};

#endif