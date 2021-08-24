SharedObject class
###################

Member Functions
----------------

SharedObject(void)

1) Sets dependent count to 0

virtual ~SharedObject(void)

1) Does nothing

virtual void AddDependent(void)

1) Increments dependent count

virtual void RemoveDependent(void)

1) Decrements dependent count if greater than 0

virtual bool HasDependents(void)

1) if dependent count > 0, return true. Otherwise, return false.

