TokenHandler class
##################

This is a simple way to provide tokens for various reasons.

Member Functions
================

TokenHandler(void)
------------------

1. Initialize the index to -1
2. create the access semaphore
3. create the exclude list with no items

~TokenHandler(void)
-------------------

1. delete the access lock
2. call ResetExcludes and delete the exclude list

int32 GetToken(void)
--------------------

Returns a unique token which is not equal to any excluded values

1. create a local variable to return the new token
2. acquire the access semaphore
3. Increment the internal index
4. while IsExclude(index) is true, increment the index
5. assign it to the local variable
6. release the access semaphore
7. return the local variable

void Reset(void)
----------------

1. acquire the access semaphore
2. set the internal index to -1
3. release the access semaphore

void ExcludeValue(int32 value)
------------------------------

1. acquire the access semaphore
2. if IsExclude(value) is false, add it to the exclude list
3. release the access semaphore

void ResetExcludes(void)
------------------------

1. acquire the access semaphore
2. Iterate through the exclude list, removing and deleting each item
3. release the access semaphore

bool IsExclude(int32 value)
---------------------------

1. create a boolean match flag and set it to false
2. acquire the access semaphore
3. iterate through the exclude list and see if the value matches any in
   the list
4. If there is a match, set the match flag to true and exit the loop
5. release the access semaphore
6. return the match flag

