//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
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
//	File Name:		Archivable.cpp
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	BArchivable mix-in class defines the archiving
//					protocol.  Also some global archiving functions.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <typeinfo>
#include <vector>

// System Includes -------------------------------------------------------------
#include <AppFileInfo.h>
#include <Archivable.h>
#include <Entry.h>
#include <List.h>
#include <OS.h>
#include <Path.h>
//#include <Roster.h>
#include <String.h>
#include <SupportDefs.h>
#include <syslog.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


using std::string;
using std::vector;

const char* B_CLASS_FIELD = "class";
const char* B_ADD_ON_FIELD = "add_on";
const int32 FUNC_NAME_LEN = 1024;

// TODO: consider moving these
//		 to a separate module, and making them more full-featured (e.g., taking
//		 NS::ClassName::Function(Param p) instead of just NS::ClassName)
static void Demangle(const char *name, BString &out);
static void Mangle(const char *name, BString &out);
// static instantiation_func FindFuncInImage(BString& funcName, image_id id,
// 										  status_t& err);
// static bool CheckSig(const char* sig, image_info& info);

/*
// TODO: Where do these get triggered from?
Log entries graciously coughed up by the Be implementation:
	Nov 28 01:40:45 instantiate_object failed: NULL BMessage argument 
	Nov 28 01:40:45 instantiate_object failed: Failed to find an entrydefining the class name (Name not found). 
	Nov 28 01:40:45 instantiate_object failed: No signature specified in archive, looking for class "TInvalidClassName". 
	Nov 28 01:40:45 instantiate_object failed: Error finding app with signature "application/x-vnd.InvalidSignature" (Application could not be found) 
	Nov 28 01:40:45 instantiate_object failed: Application could not be found (8000200b) 
	Nov 28 01:40:45 instantiate_object failed: Failed to find exported Instantiate static function for class TInvalidClassName. 
	Nov 28 01:40:45 instantiate_object failed: Invalid argument (80000005) 
	Nov 28 01:40:45 instantiate_object failed: No signature specified in archive, looking for class "TRemoteTestObject". 
	Nov 28 01:40:45 instantiate_object - couldn't get mime sig for /boot/home/src/projects/Haiku/app_kit/test/lib/support/BArchivable/./BArchivableSystemTester
	Nov 28 01:40:45 instantiate_object failed: Error finding app with signature "application/x-vnd.InvalidSignature" (Application could not be found) 
	Nov 28 01:40:45 instantiate_object failed: Application could not be found (8000200b) 
	Nov 28 01:40:45 instantiate_object failed: Error finding app with signature "application/x-vnd.InvalidSignature" (Application could not be found) 
	Nov 28 01:40:45 instantiate_object failed: Application could not be found (8000200b) 
	Nov 28 01:40:45 instantiate_object failed: Error finding app with signature "application/x-vnd.InvalidSignature" (Application could not be found) 
	Nov 28 01:40:45 instantiate_object failed: Application could not be found (8000200b) 
*/

//------------------------------------------------------------------------------
BArchivable::BArchivable()
{
	;
}
//------------------------------------------------------------------------------
BArchivable::BArchivable(BMessage* from)
{
	;
}
//------------------------------------------------------------------------------
BArchivable::~BArchivable()
{
	;
}
//------------------------------------------------------------------------------
status_t BArchivable::Archive(BMessage* into, bool deep) const
{
	if (!into)
	{
		// TODO: logging/other error reporting?
		return B_BAD_VALUE;
	}

	BString name;
	Demangle(typeid(*this).name(), name);

	return into->AddString(B_CLASS_FIELD, name);
}
//------------------------------------------------------------------------------
BArchivable* BArchivable::Instantiate(BMessage* from)
{
	debugger("Can't create a plain BArchivable object");
	return NULL;
}
//------------------------------------------------------------------------------
status_t BArchivable::Perform(perform_code d, void* arg)
{
	// TODO: Check against original
	return B_ERROR;
}
//------------------------------------------------------------------------------
void BArchivable::_ReservedArchivable1()
{
	;
}
//------------------------------------------------------------------------------
void BArchivable::_ReservedArchivable2()
{
	;
}
//------------------------------------------------------------------------------
void BArchivable::_ReservedArchivable3()
{
	;
}
//------------------------------------------------------------------------------
void BuildFuncName(const char* className, BString& funcName)
{
	funcName = "";

	//	This is what we're after:
	//		Instantiate__Q28Haiku11BArchivableP8BMessage
	Mangle(className, funcName);
	funcName.Prepend("Instantiate__");
	funcName.Append("P8BMessage");
}

//------------------------------------------------------------------------------
bool validate_instantiation(BMessage* from, const char* class_name)
{
	errno = B_OK;

	// Make sure our params are kosher -- original skimped here =P
	if (!from)
	{
		// Not standard; Be implementation has a segment
		// violation on this error mode
		errno = B_BAD_VALUE;

		return false;
	}

	status_t err = B_OK;
	const char* data;
	for (int32 index = 0; err == B_OK; ++index)
	{
		err = from->FindString(B_CLASS_FIELD, index, &data);
		if (!err && strcmp(data, class_name) == 0)
		{
			return true;
		}
	}

	errno = B_MISMATCHED_VALUES;
	syslog(LOG_ERR, "validate_instantiation failed on class %s.", class_name);

	return false;
}


//------------------------------------------------------------------------------
int GetNumber(const char*& name)
{
	int val = atoi(name);
	while (isdigit(*name))
	{
		++name;
	}

	return val;
}
//------------------------------------------------------------------------------
void Demangle(const char* name, BString& out)
{
// TODO: add support for template classes
//	_find__t12basic_string3ZcZt18string_char_traits1ZcZt24__default_alloc_template2b0i0PCccUlUl

	out = "";

	// Are we in a namespace?
	if (*name == 'Q')
	{
		// Yessir, we are; how many deep are we?
		int nsCount = 0;
		++name;
		if (*name == '_')	// more than 10 deep
		{
			++name;
			if (!isdigit(*name))
				;	// TODO: error handling

			nsCount = GetNumber(name);
			if (*name == '_')	// more than 10 deep
				++name;
			else
				;	// this should be an error condition
		}
		else
		{
			nsCount = *name - '0';
			++name;
		}

		int nameLen = 0;
		for (int i = 0; i < nsCount - 1; ++i)
		{
			if (!isdigit(*name))
				;	// TODO: error handling

			nameLen = GetNumber(name);
			out.Append(name, nameLen);
			out += "::";
			name += nameLen;
		}
	}

	out.Append(name, GetNumber(name));
}
//------------------------------------------------------------------------------
void Mangle(const char* name, BString& out)
{
// TODO: add support for template classes
//	_find__t12basic_string3ZcZt18string_char_traits1ZcZt24__default_alloc_template2b0i0PCccUlUl

	//	Chop this:
	//		testthree::testfour::Testthree::Testfour
	//	up into little bite-sized pieces
	int count = 0;
	string origName(name);
	vector<string> spacenames;

	string::size_type pos = 0;
	string::size_type oldpos = 0;
	while (pos != string::npos)
	{
		pos = origName.find_first_of("::", oldpos);
		spacenames.push_back(string(origName, oldpos, pos - oldpos));
		pos = origName.find_first_not_of("::", pos);
		oldpos = pos;
		++count;
	}

	//	Now mangle it into this:
	//		Q49testthree8testfour9Testthree8Testfour
	out = "";
	if (count > 1)
	{
		out += 'Q';
		if (count > 10)
			out += '_';
		out << count;
		if (count > 10)
			out += '_';
	}

	for (unsigned int i = 0; i < spacenames.size(); ++i)
	{
		out << (int)spacenames[i].length();
		out += spacenames[i].c_str();
	}
}


/*
 * $Log $
 *
 * $Id  $
 *
 */

