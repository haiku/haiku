//------------------------------------------------------------------------------
//	MessageRefItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEREFITEMTEST_H
#define MESSAGEREFITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Entry.h>
#include <Path.h>

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
	static status_t AddData(BMessage& msg, const char* name, type_code type,
							entry_ref* data, ssize_t size, bool);
	static status_t Find(BMessage& msg, const char* name, int32 index,
						 entry_ref* val)
	{
		return msg.FindRef(name, index, val);
	}
	static status_t ShortFind(BMessage& msg, const char* name, entry_ref* val)
	{
		return msg.FindRef(name, val);
	}
	static entry_ref QuickFind(BMessage& msg, const char* name, int32 index);
	static bool Has(BMessage& msg, const char* name, int32 index)
	{
		return msg.HasRef(name, index);
	}
	static status_t Replace(BMessage& msg, const char* name, int32 index,
							entry_ref& val)
	{
		return msg.ReplaceRef(name, index, &val);
	}
	static status_t FindData(BMessage& msg, const char* name, type_code type,
							 int32 index, const void** data, ssize_t* size);

	private:
		static entry_ref sRef;
};
status_t TRefFuncPolicy::AddData(BMessage& msg, const char* name, type_code type,
								 entry_ref* data, ssize_t size, bool)
{
	BPath Path(data);
	status_t err = Path.InitCheck();
	char* buf = NULL;
//	if (!err)
	{
		buf = new char[size];
		err = Path.Flatten(buf, size);
	}
	
	if (!err)
	{
		err = msg.AddData(name, type, buf, size, false);
	}
	delete[] buf;
	return err;
}

entry_ref TRefFuncPolicy::QuickFind(BMessage &msg, const char *name, int32 index)
{
	//	TODO: make cooler
	//	There must be some 1337 template voodoo way of making the inclusion
	//	of this function conditional.  In the meantime, we offer this
	//	cheesy hack
	entry_ref ref;
	msg.FindRef(name, index, &ref);
	return ref;
}

status_t TRefFuncPolicy::FindData(BMessage& msg, const char* name,
										 type_code type, int32 index,
										 const void** data, ssize_t* size)
{
	*data = NULL;
	char* ptr;
	status_t err = msg.FindData(name, type, index, (const void**)&ptr, size);
	if (!err)
	{
		BPath Path;
		err = Path.Unflatten(type, ptr, *size);
		if (!err)
		{
			if (Path.Path())
			{
				err = get_ref_for_path(Path.Path(), &sRef);
				if (!err)
				{
					*(entry_ref**)data = &sRef;
				}
			}
			else
			{
				sRef = entry_ref();
				*(entry_ref**)data = &sRef;
			}
		}
	}
	return err;
}
entry_ref TRefFuncPolicy::sRef;
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
	static size_t SizeOf(const entry_ref& data)
	{
		BPath Path(&data);
		return Path.FlattenedSize();
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
// TODO: fix
	static bool		Size(size_t size, entry_ref& ref)
	{
		return size == TRefInitPolicy::SizeOf(ref);
	}
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

