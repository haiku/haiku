//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
#include <Roster.h>
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
static instantiation_func FindFuncInImage(BString& funcName, image_id id,
										  status_t& err);
static bool CheckSig(const char* sig, image_info& info);

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
	Nov 28 01:40:45 instantiate_object - couldn't get mime sig for /boot/home/src/projects/OpenBeOS/app_kit/test/lib/support/BArchivable/./BArchivableSystemTester 
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
	//		Instantiate__Q28OpenBeOS11BArchivableP8BMessage
	Mangle(className, funcName);
	funcName.Prepend("Instantiate__");
	funcName.Append("P8BMessage");
}
//------------------------------------------------------------------------------
BArchivable* instantiate_object(BMessage* archive, image_id* id)
{
	errno = B_OK;

	// Check our params
	if (id)
	{
		*id = B_BAD_VALUE;
	}

	if (!archive)
	{
		// TODO: extended error handling
		errno = B_BAD_VALUE;
		syslog(LOG_ERR, "instantiate_object failed: NULL BMessage argument");
		return NULL;
	}

	// Get class name from archive
	const char* name = NULL;
	status_t err = archive->FindString(B_CLASS_FIELD, &name);
	if (err)
	{
		// TODO: extended error handling
		syslog(LOG_ERR, "instantiate_object failed: Failed to find an entry "
			   "defining the class name (%s).", strerror(err));
		return NULL;
	}

	// Get sig from archive
	const char* sig = NULL;
	bool hasSig = (archive->FindString(B_ADD_ON_FIELD, &sig) == B_OK);

	instantiation_func iFunc = find_instantiation_func(name, sig);

	//	if find_instantiation_func() can't locate Class::Instantiate()
	//		and a signature was specified
	if (!iFunc && hasSig)
	{
		//	use BRoster::FindApp() to locate an app or add-on with the symbol
		BRoster Roster;
		entry_ref ref;
		err = Roster.FindApp(sig, &ref);

		BEntry entry;

		//	if an entry_ref is obtained
		if (!err)
		{
			err = entry.SetTo(&ref);
		}

		if (err)
		{
			syslog(LOG_ERR, "instantiate_object failed: Error finding app "
							"with signature \"%s\" (%s)", sig, strerror(err));
		}

		if (!err)
		{
			BPath path;
			err = entry.GetPath(&path);
			if (!err)
			{
				//	load the app/add-on
				image_id theImage = load_add_on(path.Path());
				if (theImage < 0)
				{
					// TODO: extended error handling
					return NULL;
				}
		
				// Save the image_id
				if (id)
				{
					*id = theImage;
				}
		
				BString funcName;
				BuildFuncName(name, funcName);
				iFunc = FindFuncInImage(funcName, theImage, err);
				if (!iFunc)
				{
					syslog(LOG_ERR, "instantiate_object failed: Failed to find exported "
						   "Instantiate static function for class %s.", name);
				}
			}
		}
	}
	else if (!iFunc)
	{
		syslog(LOG_ERR, "instantiate_object failed: No signature specified "
			   "in archive, looking for class \"%s\".", name);
		errno = B_BAD_VALUE;
	}

	if (err)
	{
		// TODO: extended error handling
		syslog(LOG_ERR, "instantiate_object failed: %s (%x)",
			   strerror(err), err);
		errno = err;
		return NULL;
	}

	//	if Class::Instantiate(BMessage*) was found
	if (iFunc)
	{
		//	use to create and return an object instance
		return iFunc(archive);
	}

	return NULL;
}
//------------------------------------------------------------------------------
BArchivable* instantiate_object(BMessage* from)
{
	return instantiate_object(from, NULL);
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
instantiation_func find_instantiation_func(const char* class_name,
										   const char* sig)
{
	errno = B_OK;
	if (!class_name)
	{
		errno = B_BAD_VALUE;
		return NULL;
	}

	BRoster Roster;
	instantiation_func theFunc = NULL;
	BString funcName;

	BuildFuncName(class_name, funcName);

	thread_id tid = find_thread(NULL);
	thread_info ti;
	status_t err = get_thread_info(tid, &ti);
	if (!err)
	{
		//	for each image_id in team_id
		image_info info;
		int32 cookie = 0;
		while (!theFunc && (get_next_image_info(ti.team, &cookie, &info) == B_OK))
		{
			theFunc = FindFuncInImage(funcName, info.id, err);
		}
	
		if (theFunc && !CheckSig(sig, info))
		{
			// TODO: extended error handling
			theFunc = NULL;
		}
	}

	return theFunc;
}
//------------------------------------------------------------------------------
instantiation_func find_instantiation_func(const char* class_name)
{
	return find_instantiation_func(class_name, NULL);
}
//------------------------------------------------------------------------------
instantiation_func find_instantiation_func(BMessage* archive_data)
{
	errno = B_OK;

	if (!archive_data)
	{
		// TODO:  extended error handling
		errno = B_BAD_VALUE;
		return NULL;
	}

	const char* name = NULL;
	const char* sig = NULL;
	status_t err;

	err = archive_data->FindString(B_CLASS_FIELD, &name);
	if (err)
	{
		// TODO:  extended error handling
		return NULL;
	}

	err = archive_data->FindString(B_ADD_ON_FIELD, &sig);

	return find_instantiation_func(name, sig);
}
//------------------------------------------------------------------------------


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
//------------------------------------------------------------------------------
instantiation_func FindFuncInImage(BString& funcName, image_id id,
								   status_t& err)
{
	err = B_OK;
	instantiation_func theFunc = NULL;

	// Don't need to do it this way ...
#if 0
	char foundFuncName[FUNC_NAME_LEN];
	int32 symbolType;
	int32 funcNameLen;

	//	for each B_SYMBOL_TYPE_TEXT in image_id
	for (int32 i = 0; !err; ++i)
	{
		funcNameLen = FUNC_NAME_LEN;
		err = get_nth_image_symbol(id, i, foundFuncName, &funcNameLen,
								   &symbolType, (void**)&theFunc);

		if (!err && symbolType == B_SYMBOL_TYPE_TEXT)
		{
			//	try to match Class::Instantiate(BMessage*) signature
			if (funcName.ICompare(foundFuncName, funcNameLen) == 0)
				break;
			else
				theFunc = NULL;
		}
	}
#endif

	err = get_image_symbol(id, funcName.String(), B_SYMBOL_TYPE_TEXT,
						   (void**)&theFunc);

	if (err)
	{
		// TODO: error handling
		theFunc = NULL;
	}

	return theFunc;
}
//------------------------------------------------------------------------------
bool CheckSig(const char* sig, image_info& info)
{
	if (!sig)
	{
		// If it wasn't specified, anything "matches"
		return true;
	}

	status_t err = B_OK;

	// Get image signature
	BFile ImageFile(info.name, B_READ_ONLY);
	err = ImageFile.InitCheck();
	if (err)
	{
		// TODO: extended error handling
		return false;
	}

	char imageSig[B_MIME_TYPE_LENGTH];
	BAppFileInfo AFI(&ImageFile);
	err = AFI.GetSignature(imageSig);
	if (err)
	{
		// TODO: extended error handling
		syslog(LOG_ERR, "instantiate_object - couldn't get mime sig for %s",
			   info.name);
		return false;
	}

	return strcmp(sig, imageSig) == 0;
}
//------------------------------------------------------------------------------


/*
 * $Log $
 *
 * $Id  $
 *
 */

