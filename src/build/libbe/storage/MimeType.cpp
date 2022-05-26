//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file MimeType.cpp
	BMimeType implementation.
*/
#include "MimeType.h"

#include <Bitmap.h>

#include <ctype.h>			// For tolower()
#include <new>			// For new(nothrow)
#include <stdio.h>			// For printf()
#include <string.h>			// For strncpy()

using namespace BPrivate;

// Private helper functions
bool isValidMimeChar(const char ch);
status_t toLower(const char *str, char *result);

//using namespace BPrivate::Storage::Mime;
using namespace std;

const char *B_PEF_APP_MIME_TYPE		= "application/x-be-executable";
const char *B_PE_APP_MIME_TYPE		= "application/x-vnd.be-peexecutable";
const char *B_ELF_APP_MIME_TYPE		= "application/x-vnd.be-elfexecutable";
const char *B_RESOURCE_MIME_TYPE	= "application/x-be-resource";
const char *B_FILE_MIME_TYPE		= "application/octet-stream";
// Might be defined platform depended, but ELF will certainly be the common
// format for all platforms anyway.
const char *B_APP_MIME_TYPE			= B_ELF_APP_MIME_TYPE;

// constructor
/*!	\brief Creates an uninitialized BMimeType object.
*/
BMimeType::BMimeType()
	: fType(NULL)
	, fCStatus(B_NO_INIT)
{
}

// constructor
/*!	\brief Creates a BMimeType object and initializes it to the supplied
	MIME type.
	The supplied string must specify a valid MIME type or supertype.
	\see SetTo() for further information.
	\param mimeType The MIME string.
*/
BMimeType::BMimeType(const char *mimeType)
	: fType(NULL)
	, fCStatus(B_NO_INIT)
{
	SetTo(mimeType);
}

// destructor
/*!	\brief Frees all resources associated with this object.
*/
BMimeType::~BMimeType()
{
	Unset();
}

// SetTo
/*!	\brief Initializes this object to the supplied MIME type.
	The supplied string must specify a valid MIME type or supertype.
	Valid MIME types are given by the following grammar:
	MIMEType	::= Supertype "/" [ Subtype ]
	Supertype	::= "application" | "audio" | "image" | "message"
					| "multipart" | "text" | "video"
	Subtype		::= MIMEChar MIMEChar*
	MIMEChar	::= any character except white spaces, CTLs and '/', '<', '>',
					'@',, ',', ';', ':', '"', '(', ')', '[', ']', '?', '=', '\'
					(Note: RFC1341 also forbits '.', but it is allowed here.)

	Currently the supertype is not restricted to one of the seven types given,
	but can be an arbitrary string (obeying the same rule as the subtype).
	Nevertheless it is a very bad idea to use another supertype.
	The supplied MIME string is copied; the caller retains the ownership.
	\param mimeType The MIME string.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL or invalid \a mimeString.
	- \c B_NO_MEMORY: Insufficient memory to copy the MIME string.
*/
status_t
BMimeType::SetTo(const char *mimeType)
{
	if (!mimeType || !BMimeType::IsValid(mimeType)) {
		fCStatus = B_BAD_VALUE;
	} else {
		Unset();
		fType = new(std::nothrow) char[strlen(mimeType)+1];
		if (fType) {
			strcpy(fType, mimeType);
			fCStatus = B_OK;
		} else {
			fCStatus = B_NO_MEMORY;
		}
	}
	return fCStatus;
}

// Unset
/*!	\brief Returns the object to an uninitialized state.
*/
void
BMimeType::Unset()
{
	if (fType)
		delete [] fType;
	fType = NULL;
	fCStatus = B_NO_INIT;
}

// InitCheck
/*!	Returns the result of the most recent constructor or SetTo() call.
	\return
	- \c B_OK: The object is properly initialized.
	- A specific error code otherwise.
*/
status_t
BMimeType::InitCheck() const
{
	return fCStatus;
}

// Type
/*!	\brief Returns the MIME string represented by this object.
	\return The MIME string, if the object is properly initialized, \c NULL
			otherwise.
*/
const char *
BMimeType::Type() const
{
	return fType;
}

// IsValid
/*!	\brief Returns whether the object represents a valid MIME type.
	\see SetTo() for further information.
	\return \c true, if the object is properly initialized, \c false
			otherwise.
*/
bool
BMimeType::IsValid() const
{
	return InitCheck() == B_OK && BMimeType::IsValid(Type());
}

// IsSupertypeOnly
/*!	\brief Returns whether this objects represents a supertype.
	\return \c true, if the object is properly initialized and represents a
			supertype, \c false otherwise.
*/
bool
BMimeType::IsSupertypeOnly() const
{
	if (fCStatus == B_OK) {
		// We assume here fCStatus will be B_OK *only* if
		// the MIME string is valid
		int len = strlen(fType);
		for (int i = 0; i < len; i++) {
			if (fType[i] == '/')
				return false;
		}
		return true;
	} else
		return false;
}

// IsInstalled
//! Returns whether or not this type is currently installed in the MIME database
/*! To add the MIME type to the database, call \c Install().
	To remove the MIME type from the database, call \c Delete().

	\return
	- \c true: The MIME type is currently installed in the database
	- \c false: The MIME type is not currently installed in the database
*/
// bool
// BMimeType::IsInstalled() const
// {
// 	return InitCheck() == B_OK && is_installed(Type());
// }

// GetSupertype
/*!	\brief Returns the supertype of the MIME type represented by this object.
	The supplied object is initialized to this object's supertype. If this
	BMimeType is not properly initialized, the supplied object will be Unset().
	\param superType A pointer to the BMimeType object that shall be
		   initialized to this object's supertype.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a superType, this object is not initialized,
	  or this object <i> is </i> a supertype.
*/
status_t
BMimeType::GetSupertype(BMimeType *superType) const
{
	if (!superType)
		return B_BAD_VALUE;
	superType->Unset();

	status_t err = (fCStatus == B_OK ? B_OK : B_BAD_VALUE);
	if (!err) {
		int len = strlen(fType);
		int i;
		for (i = 0; i < len; i++) {
			if (fType[i] == '/')
				break;
		}
		if (i == len)
			err = B_BAD_VALUE;		// IsSupertypeOnly() == true
		else {
			char superMime[B_MIME_TYPE_LENGTH];
			strncpy(superMime, fType, i);
			superMime[i] = 0;
			err = superType->SetTo(superMime);
		}
	}
	return err;
}

// ==
/*!	\brief Returns whether this and the supplied MIME type are equal.
	Two BMimeType objects are said to be equal, if they represent the same
	MIME string, ignoring case, or if both are not initialized.
	\param type The BMimeType to be compared with.
	\return \c true, if the objects are equal, \c false otherwise.
*/
bool
BMimeType::operator==(const BMimeType &type) const
{
	char lower1[B_MIME_TYPE_LENGTH];
	char lower2[B_MIME_TYPE_LENGTH];
	
	if (InitCheck() == B_OK && type.InitCheck() == B_OK) {
		status_t err = toLower(Type(), lower1);
		if (!err)
			err = toLower(type.Type(), lower2);
		if (!err) 
			err = (strcmp(lower1, lower2) == 0 ? B_OK : B_ERROR);
		return err == B_OK;
	} else if (InitCheck() == B_NO_INIT && type.InitCheck() == B_NO_INIT) {
		return true;
	} else {
		return false;
	}
}

// ==
/*!	\brief Returns whether this and the supplied MIME type are equal.
	A BMimeType objects equals a MIME string, if its MIME string equals the
	latter one, ignoring case, or if it is uninitialized and the MIME string
	is \c NULL.
	\param type The MIME string to be compared with.
	\return \c true, if the MIME types are equal, \c false otherwise.
*/
bool
BMimeType::operator==(const char *type) const
{
	BMimeType mime;
	if (type)
		mime.SetTo(type);
	return (*this) == mime;
}

// Contains
/*!	\brief Returns whether this MIME type is a supertype of or equals the
	supplied one.
	\param type The MIME type.
	\return \c true, if this MIME type is a supertype of or equals the
			supplied one, \c false otherwise.
*/
bool
BMimeType::Contains(const BMimeType *type) const
{
	if (!type)
		return false;
	if (*this == *type)
		return true;
	BMimeType super;
	if (type->GetSupertype(&super) == B_OK && *this == super)
		return true;
	return false;	
}

// IsValid
/*!	\brief Returns whether the given string represents a valid MIME type.
	\see SetTo() for further information.
	\return \c true, if the given string represents a valid MIME type.
*/
bool
BMimeType::IsValid(const char *string)
{
	if (!string)
		return false;
		
	bool foundSlash = false;		
	int len = strlen(string);
	if (len >= B_MIME_TYPE_LENGTH || len == 0)
		return false;
		
	for (int i = 0; i < len; i++) {
		char ch = string[i];
		if (ch == '/') {
			if (foundSlash || i == 0 || i == len-1)
				return false;
			else
				foundSlash = true;
		} else if (!isValidMimeChar(ch)) {
			return false;
		}
	}	
	return true;
}

bool isValidMimeChar(const char ch)
{
	return    ch > 32		// Handles white space and most CTLs
	       && ch != '/'
	       && ch != '<'
	       && ch != '>'
	       && ch != '@'
	       && ch != ','
	       && ch != ';'
	       && ch != ':'
	       && ch != '"'
	       && ch != '('
	       && ch != ')'
	       && ch != '['
	       && ch != ']'
	       && ch != '?'
	       && ch != '='
	       && ch != '\\'
	       && ch != 127;	// DEL
}

// SetType
/*!	\brief Initializes this object to the supplied MIME type.
	\deprecated This method has the same semantics as SetTo().
				Use SetTo() instead.
*/
status_t
BMimeType::SetType(const char *mimeType)
{
	return SetTo(mimeType);
}

// GuessMimeType
status_t
BMimeType::GuessMimeType(const entry_ref *ref, BMimeType *type)
{
	if (!ref || !type)
		return B_BAD_VALUE;

	// get BEntry
	BEntry entry;
	status_t error = entry.SetTo(ref);
	if (error != B_OK)
		return error;

	// does entry exist?
	if (!entry.Exists())
		return B_ENTRY_NOT_FOUND;

	// check entry type
	if (entry.IsDirectory())
		return type->SetType("application/x-vnd.be-directory");
	if (entry.IsSymLink())
		return type->SetType("application/x-vnd.be-symlink");
	if (!entry.IsFile())
		return B_ERROR;

	// we have a file, read the first 4 bytes
	BFile file;
	char buffer[4];
	if (file.SetTo(ref, B_READ_ONLY) == B_OK
		&& file.Read(buffer, 4) == 4) {
		return GuessMimeType(buffer, 4, type);
	}

	// we couldn't open or read the file
	return type->SetType(B_FILE_MIME_TYPE);
}

// GuessMimeType
status_t
BMimeType::GuessMimeType(const void *_buffer, int32 length, BMimeType *type)
{
	const uint8 *buffer = (const uint8*)_buffer;
	if (!buffer || !type)
		return B_BAD_VALUE;

	// we only know ELF files
	if (length >= 4 && buffer[0] == 0x7f && buffer[1] == 'E' && buffer[2] == 'L'
		&&  buffer[3] == 'F') {
		return type->SetType(B_ELF_APP_MIME_TYPE);
	}

	return type->SetType(B_FILE_MIME_TYPE);
}

// GuessMimeType
status_t
BMimeType::GuessMimeType(const char *filename, BMimeType *type)
{
	if (!filename || !type)
		return B_BAD_VALUE;

	entry_ref ref;
	status_t error = get_ref_for_path(filename, &ref);
	return (error == B_OK ? GuessMimeType(&ref, type) : error);
}

void BMimeType::_ReservedMimeType1() {}
void BMimeType::_ReservedMimeType2() {}
void BMimeType::_ReservedMimeType3() {}

// =
/*!	\brief Unimplemented assignment operator.
*/
BMimeType &
BMimeType::operator=(const BMimeType &)
{
	return *this;	// not implemented
}

// copy constructor
/*!	\brief Unimplemented copy constructor.
*/
BMimeType::BMimeType(const BMimeType &)
{
}


// Returns a lowercase version of str in result. Result must
// be preallocated and is assumed to be of adequate length.
status_t
toLower(const char *str, char *result) {
	if (!str || !result)
		return B_BAD_VALUE;
	int len = strlen(str);
	int i;
	for (i = 0; i < len; i++)
		result[i] = tolower(str[i]);
	result[i] = 0;
	return B_OK;
}



