//------------------------------------------------------------------------------
//	MessageRefItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEREFITEMTEST_H
#define MESSAGEREFITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Entry.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

struct TRefFuncPolicy
{
	static status_t Add(BMessage& msg, const char* name, entry_ref& val)
	{
		return msg.AddRef(name, &val);
	}
	static status_t Find(BMessage& msg, const char* name, int32 index,
						 entry_ref* val)
	{
		return msg.FindRef(name, index, val);
	}
	static entry_ref QuickFind(BMessage& msg, const char* name, int32 index)
	{
		//	TODO: make cooler
		//	There must be some 1337 template voodoo way of making the inclusion
		//	of this function conditional.  In the meantime, we offer this
		//	cheesy hack
		entry_ref ref;
		msg.FindRef(name, index, &ref);
		return ref;
	}
	static bool Has(BMessage& msg, const char* name, int32 index)
	{
		return msg.HasRef(name, index);
	}
	static status_t Replace(BMessage& msg, const char* name, int32 index,
							entry_ref val)
	{
		return msg.ReplaceRef(name, index, &val);
	}
};
//------------------------------------------------------------------------------
struct TRefInitPolicy : public ArrayTypeBase<entry_ref>
{
	inline static entry_ref Zero()	{ return entry_ref(); }
	static entry_ref Test1()
	{
		entry_ref ref;
		get_ref_for_path("/boot/beos/apps/camera", &ref);
		return ref;
	}
	static entry_ref Test2()
	{
		entry_ref ref;
		get_ref_for_path("/boot/develop/headers/be/Be.h", &ref);
		return ref;
	}
	inline static ArrayType Array()
	{
		ArrayType array;
		array.push_back(Zero());
		array.push_back(Test1());
		array.push_back(Test2());
		return array;
	}
};
//------------------------------------------------------------------------------
struct TRefAssertPolicy
{
	inline static entry_ref Zero()		{ return TRefInitPolicy::Zero(); }
	inline static entry_ref Invalid()	{ return TRefInitPolicy::Zero();}
};
//------------------------------------------------------------------------------
typedef TMessageItemTest
<
	entry_ref,
	B_REF_TYPE,
	TRefFuncPolicy,
	TRefInitPolicy,
	TRefAssertPolicy
>
TMessageRefItemTest;

#endif	// MESSAGEREFITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

