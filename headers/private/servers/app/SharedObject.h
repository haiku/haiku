//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		SharedObject.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	This class is used as a base class for other objects which
//  				must be shared. By using this, objects which depend on its 
//					existence can be somewhat sure that the shared object will
//					not be deleted until nothing depends on it.
//  
//------------------------------------------------------------------------------
#ifndef SHAREDOBJ_H_
#define SHAREDOBJ_H_

#include <SupportDefs.h>

/*!
	\class SharedObject SharedObject.h
	\brief Base class for shared objects
	
	SharedObjects track dependencies upon a particular object. In this way, is is
	possible to ensure that a shared resource is not deleted if something else
	needs it. How the dependency tracking is done largely depends on the child class.
*/
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
