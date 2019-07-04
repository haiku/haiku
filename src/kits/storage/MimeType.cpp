/*
 * Copyright 2002-2006 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 */


#include "MimeType.h"

#include <Bitmap.h>
#include <mime/database_support.h>
#include <mime/DatabaseLocation.h>
#include <sniffer/Rule.h>
#include <sniffer/Parser.h>

#include <RegistrarDefs.h>
#include <RosterPrivate.h>

#include <ctype.h>
#include <new>
#include <stdio.h>
#include <strings.h>


using namespace BPrivate;

// Private helper functions
static bool isValidMimeChar(const char ch);

using namespace BPrivate::Storage::Mime;
using namespace std;

const char* B_PEF_APP_MIME_TYPE		= "application/x-be-executable";
const char* B_PE_APP_MIME_TYPE		= "application/x-vnd.Be-peexecutable";
const char* B_ELF_APP_MIME_TYPE		= "application/x-vnd.Be-elfexecutable";
const char* B_RESOURCE_MIME_TYPE	= "application/x-be-resource";
const char* B_FILE_MIME_TYPE		= "application/octet-stream";
// Might be defined platform depended, but ELF will certainly be the common
// format for all platforms anyway.
const char* B_APP_MIME_TYPE			= B_ELF_APP_MIME_TYPE;


static bool
isValidMimeChar(const char ch)
{
	// Handles white space and most CTLs
	return ch > 32
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


//	#pragma mark -


// Creates an uninitialized BMimeType object.
BMimeType::BMimeType()
	:
	fType(NULL),
	fCStatus(B_NO_INIT)
{
}


// Creates a BMimeType object and initializes it to the supplied
// MIME type.
BMimeType::BMimeType(const char* mimeType)
	:
	fType(NULL),
	fCStatus(B_NO_INIT)
{
	SetTo(mimeType);
}


// Frees all resources associated with this object.
BMimeType::~BMimeType()
{
	Unset();
}


// Initializes this object to the supplied MIME type.
status_t
BMimeType::SetTo(const char* mimeType)
{
	if (mimeType == NULL) {
		Unset();
	} else if (!BMimeType::IsValid(mimeType)) {
		fCStatus = B_BAD_VALUE;
	} else {
		Unset();
		fType = new(std::nothrow) char[strlen(mimeType) + 1];
		if (fType) {
			strlcpy(fType, mimeType, B_MIME_TYPE_LENGTH);
			fCStatus = B_OK;
		} else {
			fCStatus = B_NO_MEMORY;
		}
	}
	return fCStatus;
}


// Returns the object to an uninitialized state
void
BMimeType::Unset()
{
	delete [] fType;
	fType = NULL;
	fCStatus = B_NO_INIT;
}


// Returns the result of the most recent constructor or SetTo() call
status_t
BMimeType::InitCheck() const
{
	return fCStatus;
}


// Returns the MIME string represented by this object
const char*
BMimeType::Type() const
{
	return fType;
}


// Returns whether the object represents a valid MIME type
bool
BMimeType::IsValid() const
{
	return InitCheck() == B_OK && BMimeType::IsValid(Type());
}


// Returns whether this objects represents a supertype
bool
BMimeType::IsSupertypeOnly() const
{
	if (fCStatus == B_OK) {
		// We assume here fCStatus will be B_OK *only* if
		// the MIME string is valid
		size_t len = strlen(fType);
		for (size_t i = 0; i < len; i++) {
			if (fType[i] == '/')
				return false;
		}
		return true;
	} else
		return false;
}


// Returns whether or not this type is currently installed in the
// MIME database
bool
BMimeType::IsInstalled() const
{
	return InitCheck() == B_OK
		&& default_database_location()->IsInstalled(Type());
}


// Gets the supertype of the MIME type represented by this object
status_t
BMimeType::GetSupertype(BMimeType* supertype) const
{
	if (supertype == NULL)
		return B_BAD_VALUE;

	supertype->Unset();
	status_t status = fCStatus == B_OK ? B_OK : B_BAD_VALUE;
	if (status == B_OK) {
		size_t len = strlen(fType);
		size_t i = 0;
		for (; i < len; i++) {
			if (fType[i] == '/')
				break;
		}
		if (i == len) {
			// object is a supertype only
			status = B_BAD_VALUE;
		} else {
			char superMime[B_MIME_TYPE_LENGTH];
			strncpy(superMime, fType, i);
			superMime[i] = 0;
			status = supertype->SetTo(superMime) == B_OK ? B_OK : B_BAD_VALUE;
		}
	}

	return status;
}


// Returns whether this and the supplied MIME type are equal
bool
BMimeType::operator==(const BMimeType &type) const
{
	if (InitCheck() == B_NO_INIT && type.InitCheck() == B_NO_INIT)
		return true;
	else if (InitCheck() == B_OK && type.InitCheck() == B_OK)
		return strcasecmp(Type(), type.Type()) == 0;

	return false;
}


// Returns whether this and the supplied MIME type are equal
bool
BMimeType::operator==(const char* type) const
{
	BMimeType mime;
	if (type)
		mime.SetTo(type);

	return (*this) == mime;
}


// Returns whether this MIME type is a supertype of or equals the
// supplied one
bool
BMimeType::Contains(const BMimeType* type) const
{
	if (type == NULL)
		return false;

	if (*this == *type)
		return true;

	BMimeType super;
	if (type->GetSupertype(&super) == B_OK && *this == super)
		return true;
	return false;
}


// Adds the MIME type to the MIME database
status_t
BMimeType::Install()
{
	status_t err = InitCheck();

	BMessage message(B_REG_MIME_INSTALL);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddString("type", Type());

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	return err;
}


// Removes the MIME type from the MIME database
status_t
BMimeType::Delete()
{
	status_t err = InitCheck();

	BMessage message(B_REG_MIME_DELETE);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddString("type", Type());

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	return err;
}


// Fetches the large or mini icon associated with the MIME type
status_t
BMimeType::GetIcon(BBitmap* icon, icon_size size) const
{
	if (icon == NULL)
		return B_BAD_VALUE;

	status_t err = InitCheck();
	if (err == B_OK)
		err = default_database_location()->GetIcon(Type(), *icon, size);

	return err;
}


//	Fetches the vector icon associated with the MIME type
status_t
BMimeType::GetIcon(uint8** data, size_t* size) const
{
	if (data == NULL || size == NULL)
		return B_BAD_VALUE;

	status_t err = InitCheck();
	if (err == B_OK)
		err = default_database_location()->GetIcon(Type(), *data, *size);

	return err;
}


// Fetches the signature of the MIME type's preferred application from the
// MIME database
status_t
BMimeType::GetPreferredApp(char* signature, app_verb verb) const
{
	status_t err = InitCheck();
	if (err == B_OK) {
		err = default_database_location()->GetPreferredApp(Type(), signature,
			verb);
	}

	return err;
}


// Fetches from the MIME database a BMessage describing the attributes
// typically associated with files of the given MIME type
status_t
BMimeType::GetAttrInfo(BMessage* info) const
{
	if (info == NULL)
		return B_BAD_VALUE;

	status_t err = InitCheck();
	if (err == B_OK)
		err = default_database_location()->GetAttributesInfo(Type(), *info);

	return err;
}


// Fetches the MIME type's associated filename extensions from the MIME
// database
status_t
BMimeType::GetFileExtensions(BMessage* extensions) const
{
	if (extensions == NULL)
		return B_BAD_VALUE;

	status_t err = InitCheck();
	if (err == B_OK) {
		err = default_database_location()->GetFileExtensions(Type(),
			*extensions);
	}

	return err;
}


// Fetches the MIME type's short description from the MIME database
status_t
BMimeType::GetShortDescription(char* description) const
{
	status_t err = InitCheck();
	if (err == B_OK) {
		err = default_database_location()->GetShortDescription(Type(),
			description);
	}

	return err;
}


// Fetches the MIME type's long description from the MIME database
status_t
BMimeType::GetLongDescription(char* description) const
{
	status_t err = InitCheck();
	if (err == B_OK) {
		err = default_database_location()->GetLongDescription(Type(),
			description);
	}

	return err;
}


// Fetches a \c BMessage containing a list of MIME signatures of
// applications that are able to handle files of this MIME type.
status_t
BMimeType::GetSupportingApps(BMessage* signatures) const
{
	if (signatures == NULL)
		return B_BAD_VALUE;

	BMessage message(B_REG_MIME_GET_SUPPORTING_APPS);
	status_t result;

	status_t err = InitCheck();
	if (err == B_OK)
		err = message.AddString("type", Type());
	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, signatures, true);
	if (err == B_OK) {
		err = (status_t)(signatures->what == B_REG_RESULT ? B_OK
			: B_BAD_REPLY);
	}
	if (err == B_OK)
		err = signatures->FindInt32("result", &result);
	if (err == B_OK)
		err = result;

	return err;
}


// Sets the large or mini icon for the MIME type
status_t
BMimeType::SetIcon(const BBitmap* icon, icon_size which)
{
	return SetIconForType(NULL, icon, which);
}


// Sets the vector icon for the MIME type
status_t
BMimeType::SetIcon(const uint8* data, size_t size)
{
	return SetIconForType(NULL, data, size);
}


// Sets the preferred application for the MIME type
status_t
BMimeType::SetPreferredApp(const char* signature, app_verb verb)
{
	status_t err = InitCheck();

	BMessage message(signature && signature[0]
		? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddString("type", Type());

	if (err == B_OK)
		err = message.AddInt32("which", B_REG_MIME_PREFERRED_APP);

	if (err == B_OK && signature != NULL)
		err = message.AddString("signature", signature);

	if (err == B_OK)
		err = message.AddInt32("app verb", verb);

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	return err;
}


// Sets the description of the attributes typically associated with files
// of the given MIME type
status_t
BMimeType::SetAttrInfo(const BMessage* info)
{
	status_t err = InitCheck();

	BMessage message(info ? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddString("type", Type());
	if (err == B_OK)
		err = message.AddInt32("which", B_REG_MIME_ATTR_INFO);
	if (err == B_OK && info != NULL)
		err = message.AddMessage("attr info", info);
	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);
	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (err == B_OK)
		err = reply.FindInt32("result", &result);
	if (err == B_OK)
		err = result;

	return err;
}


// Sets the list of filename extensions associated with the MIME type
status_t
BMimeType::SetFileExtensions(const BMessage* extensions)
{
	status_t err = InitCheck();

	BMessage message(extensions ? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddString("type", Type());

	if (err == B_OK)
		err = message.AddInt32("which", B_REG_MIME_FILE_EXTENSIONS);

	if (err == B_OK && extensions != NULL)
		err = message.AddMessage("extensions", extensions);

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	return err;
}


// Sets the short description field for the MIME type
status_t
BMimeType::SetShortDescription(const char* description)
{
	status_t err = InitCheck();

	BMessage message(description && description [0]
		? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddString("type", Type());

	if (err == B_OK)
		err = message.AddInt32("which", B_REG_MIME_DESCRIPTION);

	if (err == B_OK && description)
		err = message.AddString("description", description);

	if (err == B_OK)
		err = message.AddBool("long", false);

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	return err;
}


// Sets the long description field for the MIME type
status_t
BMimeType::SetLongDescription(const char* description)
{
	status_t err = InitCheck();

	BMessage message(description && description[0]
		? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddString("type", Type());

	if (err == B_OK)
		err = message.AddInt32("which", B_REG_MIME_DESCRIPTION);

	if (err == B_OK && description)
		err = message.AddString("description", description);

	if (err == B_OK)
		err = message.AddBool("long", true);

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	return err;
}


// Fetches a BMessage listing all the MIME supertypes currently
// installed in the MIME database.
/*static*/ status_t
BMimeType::GetInstalledSupertypes(BMessage* supertypes)
{
	if (supertypes == NULL)
		return B_BAD_VALUE;

	BMessage message(B_REG_MIME_GET_INSTALLED_SUPERTYPES);
	status_t result;

	status_t err = BRoster::Private().SendTo(&message, supertypes, true);
	if (err == B_OK) {
		err = (status_t)(supertypes->what == B_REG_RESULT ? B_OK
			: B_BAD_REPLY);
	}
	if (err == B_OK)
		err = supertypes->FindInt32("result", &result);
	if (err == B_OK)
		err = result;

	return err;
}


// Fetches a BMessage listing all the MIME types currently installed
// in the MIME database.
status_t
BMimeType::GetInstalledTypes(BMessage* types)
{
	return GetInstalledTypes(NULL, types);
}


// Fetches a BMessage listing all the MIME subtypes of the given
// supertype currently installed in the MIME database.
/*static*/ status_t
BMimeType::GetInstalledTypes(const char* supertype, BMessage* types)
{
	if (types == NULL)
		return B_BAD_VALUE;

	status_t result;

	// Build and send the message, read the reply
	BMessage message(B_REG_MIME_GET_INSTALLED_TYPES);
	status_t err = B_OK;

	if (supertype != NULL)
		err = message.AddString("supertype", supertype);
	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, types, true);
	if (err == B_OK)
		err = (status_t)(types->what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (err == B_OK)
		err = types->FindInt32("result", &result);
	if (err == B_OK)
		err = result;

	return err;
}


// Fetches a \c BMessage containing a list of MIME signatures of
// applications that are able to handle files of any type.
status_t
BMimeType::GetWildcardApps(BMessage* wild_ones)
{
	BMimeType mime;
	status_t err = mime.SetTo("application/octet-stream");
	if (err == B_OK)
		err = mime.GetSupportingApps(wild_ones);
	return err;
}


// Returns whether the given string represents a valid MIME type.
bool
BMimeType::IsValid(const char* string)
{
	if (string == NULL)
		return false;

	bool foundSlash = false;
	size_t len = strlen(string);
	if (len >= B_MIME_TYPE_LENGTH || len == 0)
		return false;

	for (size_t i = 0; i < len; i++) {
		char ch = string[i];
		if (ch == '/') {
			if (foundSlash || i == 0 || i == len - 1)
				return false;
			else
				foundSlash = true;
		} else if (!isValidMimeChar(ch)) {
			return false;
		}
	}
	return true;
}


// Fetches an \c entry_ref that serves as a hint as to where the MIME type's
// preferred application might live
status_t
BMimeType::GetAppHint(entry_ref* ref) const
{
	if (ref == NULL)
		return B_BAD_VALUE;

	status_t err = InitCheck();
	if (err == B_OK)
		err = default_database_location()->GetAppHint(Type(), *ref);
	return err;
}


// Sets the app hint field for the MIME type
status_t
BMimeType::SetAppHint(const entry_ref* ref)
{
	status_t err = InitCheck();

	BMessage message(ref ? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddString("type", Type());

	if (err == B_OK)
		err = message.AddInt32("which", B_REG_MIME_APP_HINT);

	if (err == B_OK && ref != NULL)
		err = message.AddRef("app hint", ref);

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	return err;
}


// Fetches the large or mini icon used by an application of this type for
// files of the given type.
status_t
BMimeType::GetIconForType(const char* type, BBitmap* icon, icon_size which) const
{
	if (icon == NULL)
		return B_BAD_VALUE;

	// If type is NULL, this function works just like GetIcon(), othewise,
	// we need to make sure the give type is valid.
	status_t err;
	if (type) {
		err = BMimeType::IsValid(type) ? B_OK : B_BAD_VALUE;
		if (err == B_OK) {
			err = default_database_location()->GetIconForType(Type(), type,
				*icon, which);
		}
	} else
		err = GetIcon(icon, which);

	return err;
}


// Fetches the vector icon used by an application of this type for files of
// the given type.
status_t
BMimeType::GetIconForType(const char* type, uint8** _data, size_t* _size) const
{
	if (_data == NULL || _size == NULL)
		return B_BAD_VALUE;

	// If type is NULL, this function works just like GetIcon(), otherwise,
	// we need to make sure the give type is valid.
	if (type == NULL)
		return GetIcon(_data, _size);

	if (!BMimeType::IsValid(type))
		return B_BAD_VALUE;

	return default_database_location()->GetIconForType(Type(), type, *_data,
		*_size);
}


// Sets the large or mini icon used by an application of this type for
// files of the given type.
status_t
BMimeType::SetIconForType(const char* type, const BBitmap* icon, icon_size which)
{
	status_t err = InitCheck();

	BMessage message(icon ? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	void* data = NULL;
	int32 dataSize;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddString("type", Type());

	if (err == B_OK) {
		err = message.AddInt32("which",
			type ? B_REG_MIME_ICON_FOR_TYPE : B_REG_MIME_ICON);
	}

	if (icon != NULL) {
		if (err == B_OK)
			err = get_icon_data(icon, which, &data, &dataSize);

		if (err == B_OK)
			err = message.AddData("icon data", B_RAW_TYPE, data, dataSize);
	}

	if (err == B_OK)
		err = message.AddInt32("icon size", which);

	if (type != NULL) {
		if (err == B_OK)
			err = BMimeType::IsValid(type) ? B_OK : B_BAD_VALUE;

		if (err == B_OK)
			err = message.AddString("file type", type);
	}

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	delete[] (int8*)data;

	return err;
}


// Sets the large or mini icon used by an application of this type for
// files of the given type.
status_t
BMimeType::SetIconForType(const char* type, const uint8* data, size_t dataSize)
{
	status_t err = InitCheck();

	BMessage message(data ? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddString("type", Type());
	if (err == B_OK)
		err = message.AddInt32("which", (type ? B_REG_MIME_ICON_FOR_TYPE : B_REG_MIME_ICON));
	if (data) {
		if (err == B_OK)
			err = message.AddData("icon data", B_RAW_TYPE, data, dataSize);
	}
	if (err == B_OK)
		err = message.AddInt32("icon size", -1);
		// -1 indicates size should be ignored (vector icon data)
	if (type) {
		if (err == B_OK)
			err = BMimeType::IsValid(type) ? B_OK : B_BAD_VALUE;
		if (err == B_OK)
			err = message.AddString("file type", type);
	}
	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);
	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (err == B_OK)
		err = reply.FindInt32("result", &result);
	if (err == B_OK)
		err = result;

	return err;
}


// Retrieves the MIME type's sniffer rule
status_t
BMimeType::GetSnifferRule(BString* result) const
{
	if (result == NULL)
		return B_BAD_VALUE;

	status_t err = InitCheck();
	if (err == B_OK)
		err = default_database_location()->GetSnifferRule(Type(), *result);

	return err;
}


// Sets the MIME type's sniffer rule
status_t
BMimeType::SetSnifferRule(const char* rule)
{
	status_t err = InitCheck();
	if (err == B_OK && rule != NULL && rule[0] != '\0')
		err = CheckSnifferRule(rule, NULL);

	if (err != B_OK)
		return err;

	BMessage message(rule && rule[0] ? B_REG_MIME_SET_PARAM
		: B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	err = message.AddString("type", Type());
	if (err == B_OK)
		err = message.AddInt32("which", B_REG_MIME_SNIFFER_RULE);

	if (err == B_OK && rule)
		err = message.AddString("sniffer rule", rule);

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	return err;
}


// Checks whether a MIME sniffer rule is valid or not.
status_t
BMimeType::CheckSnifferRule(const char* rule, BString* parseError)
{
	BPrivate::Storage::Sniffer::Rule snifferRule;

	return BPrivate::Storage::Sniffer::parse(rule, &snifferRule, parseError);
}


// Guesses a MIME type for the entry referred to by the given
// entry_ref.
status_t
BMimeType::GuessMimeType(const entry_ref* file, BMimeType* type)
{
	status_t err = file && type ? B_OK : B_BAD_VALUE;

	BMessage message(B_REG_MIME_SNIFF);
	BMessage reply;
	status_t result;
	const char* str;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddRef("file ref", file);

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	if (err == B_OK)
		err = reply.FindString("mime type", &str);

	if (err == B_OK)
		err = type->SetTo(str);

	return err;
}


// Guesses a MIME type for the supplied chunk of data.
status_t
BMimeType::GuessMimeType(const void* buffer, int32 length, BMimeType* type)
{
	status_t err = buffer && type ? B_OK : B_BAD_VALUE;

	BMessage message(B_REG_MIME_SNIFF);
	BMessage reply;
	status_t result;
	const char* str;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddData("data", B_RAW_TYPE, buffer, length);

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	if (err == B_OK)
		err = reply.FindString("mime type", &str);

	if (err == B_OK)
		err = type->SetTo(str);

	return err;
}


// Guesses a MIME type for the given filename.
status_t
BMimeType::GuessMimeType(const char* filename, BMimeType* type)
{
	status_t err = filename && type ? B_OK : B_BAD_VALUE;

	BMessage message(B_REG_MIME_SNIFF);
	BMessage reply;
	status_t result;
	const char* str;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddString("filename", filename);

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	if (err == B_OK)
		err = reply.FindString("mime type", &str);

	if (err == B_OK)
		err = type->SetTo(str);

	return err;
}


// Starts monitoring the MIME database for a given target.
status_t
BMimeType::StartWatching(BMessenger target)
{
	BMessage message(B_REG_MIME_START_WATCHING);
	BMessage reply;
	status_t result;
	status_t err;

	// Build and send the message, read the reply
	err = message.AddMessenger("target", target);
	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	return err;
}


// Stops monitoring the MIME database for a given target
status_t
BMimeType::StopWatching(BMessenger target)
{
	BMessage message(B_REG_MIME_STOP_WATCHING);
	BMessage reply;
	status_t result;
	status_t err;

	// Build and send the message, read the reply
	err = message.AddMessenger("target", target);
	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	return err;
}


// Initializes this object to the supplied MIME type
status_t
BMimeType::SetType(const char* mimeType)
{
	return SetTo(mimeType);
}


void BMimeType::_ReservedMimeType1() {}
void BMimeType::_ReservedMimeType2() {}
void BMimeType::_ReservedMimeType3() {}


#ifdef _BEOS_R5_COMPATIBLE_
// assignment operator.
// Unimplemented
BMimeType&
BMimeType::operator=(const BMimeType &)
{
	return *this;
		// not implemented
}


// copy constructor
// Unimplemented
BMimeType::BMimeType(const BMimeType &)
{
}
#endif


status_t
BMimeType::GetSupportedTypes(BMessage* types)
{
	if (types == NULL)
		return B_BAD_VALUE;

	status_t err = InitCheck();
	if (err == B_OK)
		err = default_database_location()->GetSupportedTypes(Type(), *types);

	return err;
}


/*!	Sets the list of MIME types supported by the MIME type (which is
	assumed to be an application signature).

	If \a types is \c NULL the application's supported types are unset.

	The supported MIME types must be stored in a field "types" of type
	\c B_STRING_TYPE in \a types.

	For each supported type the result of BMimeType::GetSupportingApps() will
	afterwards include the signature of this application.

	\a fullSync specifies whether or not any types that are no longer
	listed as supported types as of this call to SetSupportedTypes() shall be
	updated as well, i.e. whether this application shall be removed from their
	lists of supporting applications.

	If \a fullSync is \c false, this application will not be removed from the
	previously supported types' supporting apps lists until the next call
	to BMimeType::SetSupportedTypes() or BMimeType::DeleteSupportedTypes()
	with a \c true \a fullSync parameter, the next call to BMimeType::Delete(),
	or the next reboot.

	\param types The supported types to be assigned to the file.
	       May be \c NULL.
	\param fullSync \c true to also synchronize the previously supported
	       types, \c false otherwise.

	\returns \c B_OK on success or another error code on failure.
*/
status_t
BMimeType::SetSupportedTypes(const BMessage* types, bool fullSync)
{
	status_t err = InitCheck();

	// Build and send the message, read the reply
	BMessage message(types ? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	if (err == B_OK)
		err = message.AddString("type", Type());

	if (err == B_OK)
		err = message.AddInt32("which", B_REG_MIME_SUPPORTED_TYPES);

	if (err != B_OK && types != NULL)
		err = message.AddMessage("types", types);

	if (err == B_OK)
		err = message.AddBool("full sync", fullSync);

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	return err;
}


/*!	Returns a list of mime types associated with the given file extension

	The list of types is returned in the pre-allocated \c BMessage pointed to
	by \a types. The types are stored in the message's "types" field, which
	is an array of \c B_STRING_TYPE values.

	\param extension The file extension of interest
	\param types Pointer to a pre-allocated BMessage into which the result will
	       be stored.

	\returns \c B_OK on success or another error code on failure.
*/
status_t
BMimeType::GetAssociatedTypes(const char* extension, BMessage* types)
{
	status_t err = extension && types ? B_OK : B_BAD_VALUE;

	BMessage message(B_REG_MIME_GET_ASSOCIATED_TYPES);
	BMessage &reply = *types;
	status_t result;

	// Build and send the message, read the reply
	if (err == B_OK)
		err = message.AddString("extension", extension);

	if (err == B_OK)
		err = BRoster::Private().SendTo(&message, &reply, true);

	if (err == B_OK)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);

	if (err == B_OK)
		err = reply.FindInt32("result", &result);

	if (err == B_OK)
		err = result;

	return err;
}
