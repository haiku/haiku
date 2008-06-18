/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _AUTO_DELETE_H
#define _AUTO_DELETE_H

#include <stdlib.h>

/*
Typical usage of this class:

AClass* Klass::Method(int arg) {
	AutoDelete<AClass> variable(new AClass());
	
	if (!IsValid(arg)) {
		...
		// AutoDelete automatically deletes the AClass object.
		return NULL;
	}
	
	variable.Get()->MethodOfAClass();
	
	// Use Release() to prevent deletion of AClass object.
	return variable.Release();
}
*/

template <class Tp>
class AutoDelete {
private:
	Tp*  fObject;
	bool fOwnsObject; 

	// Deletes the object if it owns it
	void Delete() 
	{
		if (fOwnsObject) {
			delete fObject; fObject = NULL;
		}
	}

public:

	// Sets the object the class owns	
	AutoDelete(Tp* object = NULL) : fObject(object), fOwnsObject(true) { }
	
	// Deletes the object if it owns it
	virtual ~AutoDelete() 
	{
		Delete();
	}

	// Sets the object the class owns.
	// Deletes a previously owned object and
	// sets the owning flag for the new object.
	void Set(Tp* object) 
	{
		if (fObject == object) return;
		
		Delete();
		fOwnsObject = true;
		fObject = object;
	}
	
	// Returns the object
	Tp* Get() 
	{ 
		return fObject; 
	}
	
	// Returns the object and sets owning to false
	// The Get method can still be used to retrieve the object.
	Tp* Release() 
	{
		fOwnsObject = false;
		return fObject;
	}
	
	// Sets the owning flag
	void SetOwnsObject(bool ownsObject) 
	{
		fOwnsObject = ownsObject;
	}
};


#endif
