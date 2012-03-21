/*
 * Copyright 2002-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tyler Dauwalder
 *		Ingo Weinhold, bonefish@users.sf.net
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "MimeType.h"

#include <Bitmap.h>
#include <mime/database_access.h>	
#include <sniffer/Rule.h>
#include <sniffer/Parser.h>

#include <RegistrarDefs.h>
#include <RosterPrivate.h>

#include <ctype.h>
#include <new>
#include <stdio.h>
#include <string.h>


using namespace BPrivate;

// Private helper functions
static bool isValidMimeChar(const char ch);

using namespace BPrivate::Storage::Mime;
using namespace std;

const char *B_PEF_APP_MIME_TYPE		= "application/x-be-executable";
const char *B_PE_APP_MIME_TYPE		= "application/x-vnd.Be-peexecutable";
const char *B_ELF_APP_MIME_TYPE		= "application/x-vnd.Be-elfexecutable";
const char *B_RESOURCE_MIME_TYPE	= "application/x-be-resource";
const char *B_FILE_MIME_TYPE		= "application/octet-stream";
// Might be defined platform depended, but ELF will certainly be the common
// format for all platforms anyway.
const char *B_APP_MIME_TYPE			= B_ELF_APP_MIME_TYPE;


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
BMimeType::BMimeType(const char *mimeType)
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
BMimeType::SetTo(const char *mimeType)
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
	return InitCheck() == B_OK && is_installed(Type());
}


// Gets the supertype of the MIME type represented by this object
status_t
BMimeType::GetSupertype(BMimeType *superType) const
{
	if (superType == NULL)
		return B_BAD_VALUE;

	superType->Unset();
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
			status = superType->SetTo(superMime) == B_OK ? B_OK : B_BAD_VALUE;
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
BMimeType::operator==(const char *type) const
{
	BMimeType mime;
	if (type)
		mime.SetTo(type);
	return (*this) == mime;
}


// Returns whether this MIME type is a supertype of or equals the
// supplied one
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


// Adds the MIME type to the MIME database
status_t
BMimeType::Install()
{
	status_t err = InitCheck();

	BMessage msg(B_REG_MIME_INSTALL);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("type", Type());
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err) 
		err = result;

	return err;	
}


// Removes the MIME type from the MIME database
status_t
BMimeType::Delete()
{
	status_t err = InitCheck();

	BMessage msg(B_REG_MIME_DELETE);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("type", Type());
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;

	return err;
}


// Fetches the large or mini icon associated with the MIME type
status_t
BMimeType::GetIcon(BBitmap *icon, icon_size size) const
{
	status_t err = InitCheck();
	if (!err)
		err = get_icon(Type(), icon, size);

	return err;
}


//	Fetches the vector icon associated with the MIME type
status_t
BMimeType::GetIcon(uint8** data, size_t* size) const
{
	status_t err = InitCheck();
	if (!err)
		err = get_icon(Type(), data, size);

	return err;
}


// Fetches the signature of the MIME type's preferred application from the
// MIME database
status_t
BMimeType::GetPreferredApp(char *signature, app_verb verb) const
{
	status_t err = InitCheck();
	if (!err)
		err = get_preferred_app(Type(), signature, verb);

	return err;
}


// Fetches from the MIME database a BMessage describing the attributes
// typically associated with files of the given MIME type
status_t
BMimeType::GetAttrInfo(BMessage *info) const
{
	status_t err = InitCheck();
	if (!err)
		err = get_attr_info(Type(), info);

	return err;
}


// Fetches the MIME type's associated filename extensions from the MIME
// database
status_t
BMimeType::GetFileExtensions(BMessage *extensions) const
{
	status_t err = InitCheck();
	if (!err)
		err = get_file_extensions(Type(), extensions);

	return err;
}


// Fetches the MIME type's short description from the MIME database
status_t
BMimeType::GetShortDescription(char *description) const
{
	status_t err = InitCheck();
	if (!err)
		err = get_short_description(Type(), description);

	return err;
}


// Fetches the MIME type's long description from the MIME database
status_t
BMimeType::GetLongDescription(char *description) const
{
	status_t err = InitCheck();
	if (!err)
		err = get_long_description(Type(), description);

	return err;
}


// Fetches a \c BMessage containing a list of MIME signatures of
// applications that are able to handle files of this MIME type.
status_t
BMimeType::GetSupportingApps(BMessage *signatures) const
{
	if (signatures == NULL)
		return B_BAD_VALUE;

	BMessage msg(B_REG_MIME_GET_SUPPORTING_APPS);
	status_t result;

	status_t err = InitCheck();
	if (!err)
		err = msg.AddString("type", Type());
	if (!err)
		err = BRoster::Private().SendTo(&msg, signatures, true);
	if (!err) {
		err = (status_t)(signatures->what == B_REG_RESULT ? B_OK
			: B_BAD_REPLY);
	}
	if (!err)
		err = signatures->FindInt32("result", &result);
	if (!err) 
		err = result;

	return err;
}


// Sets the large or mini icon for the MIME type
status_t
BMimeType::SetIcon(const BBitmap *icon, icon_size which)
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
BMimeType::SetPreferredApp(const char *signature, app_verb verb)
{
	status_t err = InitCheck();

	BMessage msg(signature && signature[0]
		? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("type", Type());
	if (!err) 
		err = msg.AddInt32("which", B_REG_MIME_PREFERRED_APP);
	if (!err && signature) 
		err = msg.AddString("signature", signature);
	if (!err)
		err = msg.AddInt32("app verb", verb);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;

	return err;
}


// Sets the description of the attributes typically associated with files
// of the given MIME type
status_t
BMimeType::SetAttrInfo(const BMessage *info)
{
	status_t err = InitCheck();

	BMessage msg(info ? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("type", Type());
	if (!err) 
		err = msg.AddInt32("which", B_REG_MIME_ATTR_INFO);
	if (!err && info) 
		err = msg.AddMessage("attr info", info);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;

	return err;
}


// Sets the list of filename extensions associated with the MIME type
status_t
BMimeType::SetFileExtensions(const BMessage *extensions)
{
	status_t err = InitCheck();

	BMessage msg(extensions ? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("type", Type());
	if (!err) 
		err = msg.AddInt32("which", B_REG_MIME_FILE_EXTENSIONS);
	if (!err && extensions) 
		err = msg.AddMessage("extensions", extensions);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;

	return err;
}


// Sets the short description field for the MIME type
status_t
BMimeType::SetShortDescription(const char *description)
{
	status_t err = InitCheck();

	BMessage msg(description && description [0]
		? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("type", Type());
	if (!err)
		err = msg.AddInt32("which", B_REG_MIME_DESCRIPTION);
	if (!err && description) 
		err = msg.AddString("description", description);
	if (!err)
		err = msg.AddBool("long", false);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;

	return err;
}


// Sets the long description field for the MIME type
status_t
BMimeType::SetLongDescription(const char *description)
{
	status_t err = InitCheck();

	BMessage msg(description && description[0]
		? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("type", Type());
	if (!err)
		err = msg.AddInt32("which", B_REG_MIME_DESCRIPTION);
	if (!err && description) 
		err = msg.AddString("description", description);
	if (!err)
		err = msg.AddBool("long", true);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;

	return err;
}


// Fetches a BMessage listing all the MIME supertypes currently
// installed in the MIME database.
/*static*/ status_t
BMimeType::GetInstalledSupertypes(BMessage *supertypes)
{
	if (supertypes == NULL)
		return B_BAD_VALUE;

	BMessage msg(B_REG_MIME_GET_INSTALLED_SUPERTYPES);
	status_t result;

	status_t err = BRoster::Private().SendTo(&msg, supertypes, true);
	if (!err) {
		err = (status_t)(supertypes->what == B_REG_RESULT ? B_OK
			: B_BAD_REPLY);
	}
	if (!err)
		err = supertypes->FindInt32("result", &result);
	if (!err)
		err = result;

	return err;	
}


// Fetches a BMessage listing all the MIME types currently installed
// in the MIME database.
status_t
BMimeType::GetInstalledTypes(BMessage *types)
{
	return GetInstalledTypes(NULL, types);
}


// Fetches a BMessage listing all the MIME subtypes of the given
// supertype currently installed in the MIME database.
/*static*/ status_t
BMimeType::GetInstalledTypes(const char *supertype, BMessage *types)
{
	if (types == NULL)
		return B_BAD_VALUE;

	status_t result;

	// Build and send the message, read the reply
	BMessage msg(B_REG_MIME_GET_INSTALLED_TYPES);
	status_t err = B_OK;

	if (supertype != NULL)
		err = msg.AddString("supertype", supertype);
	if (!err)
		err = BRoster::Private().SendTo(&msg, types, true);
	if (!err)
		err = (status_t)(types->what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = types->FindInt32("result", &result);
	if (!err)
		err = result;

	return err;
}


// Fetches a \c BMessage containing a list of MIME signatures of
// applications that are able to handle files of any type.
status_t
BMimeType::GetWildcardApps(BMessage *wild_ones)
{
	BMimeType mime;
	status_t err = mime.SetTo("application/octet-stream");
	if (!err)
		err = mime.GetSupportingApps(wild_ones);
	return err;
}


// Returns whether the given string represents a valid MIME type.
bool
BMimeType::IsValid(const char *string)
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
BMimeType::GetAppHint(entry_ref *ref) const
{
	status_t err = InitCheck();
	if (!err)
		err = get_app_hint(Type(), ref);
	return err;
}


// Sets the app hint field for the MIME type
status_t
BMimeType::SetAppHint(const entry_ref *ref)
{
	status_t err = InitCheck();

	BMessage msg(ref ? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("type", Type());
	if (!err)
		err = msg.AddInt32("which", B_REG_MIME_APP_HINT);
	if (!err && ref)
		err = msg.AddRef("app hint", ref);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err) 
		err = result;

	return err;
}


// Fetches the large or mini icon used by an application of this type for
// files of the given type.
status_t
BMimeType::GetIconForType(const char *type, BBitmap *icon, icon_size which) const
{
	// If type is NULL, this function works just like GetIcon(), othewise,
	// we need to make sure the give type is valid.
	status_t err;
	if (type) {
		err = BMimeType::IsValid(type) ? B_OK : B_BAD_VALUE;
		if (!err)
			err = get_icon_for_type(Type(), type, icon, which);
	} else
		err = GetIcon(icon, which);

	return err;
}


// Fetches the vector icon used by an application of this type for files of
// the given type.
status_t
BMimeType::GetIconForType(const char *type, uint8** _data, size_t* _size) const
{
	// If type is NULL, this function works just like GetIcon(), otherwise,
	// we need to make sure the give type is valid.
	if (type == NULL)
		return GetIcon(_data, _size);

	if (!BMimeType::IsValid(type))
		return B_BAD_VALUE;

	return get_icon_for_type(Type(), type, _data, _size);
}


// Sets the large or mini icon used by an application of this type for
// files of the given type.
status_t
BMimeType::SetIconForType(const char *type, const BBitmap *icon, icon_size which)
{
	status_t err = InitCheck();

	BMessage msg(icon ? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;
	
	void *data = NULL;
	int32 dataSize;	
	
	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("type", Type());
	if (!err) 
		err = msg.AddInt32("which",
			type ? B_REG_MIME_ICON_FOR_TYPE : B_REG_MIME_ICON);
	if (icon) {
		if (!err)
			err = get_icon_data(icon, which, &data, &dataSize);
		if (!err)
			err = msg.AddData("icon data", B_RAW_TYPE, data, dataSize);
	}
	if (!err)
		err = msg.AddInt32("icon size", which);
	if (type) {
		if (!err)
			err = BMimeType::IsValid(type) ? B_OK : B_BAD_VALUE;
		if (!err)
			err = msg.AddString("file type", type);
	}
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err) 
		err = result;

	delete [] (int8*)data;

	return err;
}


// Sets the large or mini icon used by an application of this type for
// files of the given type.
status_t
BMimeType::SetIconForType(const char* type, const uint8* data, size_t dataSize)
{
	status_t err = InitCheck();

	BMessage msg(data ? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;
	
	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("type", Type());
	if (!err) 
		err = msg.AddInt32("which", (type ? B_REG_MIME_ICON_FOR_TYPE : B_REG_MIME_ICON));
	if (data) {
		if (!err)
			err = msg.AddData("icon data", B_RAW_TYPE, data, dataSize);
	}
	if (!err)
		err = msg.AddInt32("icon size", -1);
		// -1 indicates size should be ignored (vector icon data)
	if (type) {
		if (!err)
			err = BMimeType::IsValid(type) ? B_OK : B_BAD_VALUE;
		if (!err)
			err = msg.AddString("file type", type);
	}
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err) 
		err = result;

	return err;
}


// Retrieves the MIME type's sniffer rule
status_t
BMimeType::GetSnifferRule(BString *result) const
{
	status_t err = InitCheck();
	if (!err)
		err = get_sniffer_rule(Type(), result);

	return err;
}


// Sets the MIME type's sniffer rule
status_t
BMimeType::SetSnifferRule(const char *rule)
{
	status_t err = InitCheck();
	if (!err && rule && rule[0])
		err = CheckSnifferRule(rule, NULL);
	if (err != B_OK)
		return err;

	BMessage msg(rule && rule[0] ? B_REG_MIME_SET_PARAM
		: B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	err = msg.AddString("type", Type());
	if (!err)
		err = msg.AddInt32("which", B_REG_MIME_SNIFFER_RULE);
	if (!err && rule)
		err = msg.AddString("sniffer rule", rule);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;

	return err;
}


// Checks whether a MIME sniffer rule is valid or not.
status_t
BMimeType::CheckSnifferRule(const char *rule, BString *parseError)
{
	BPrivate::Storage::Sniffer::Rule snifferRule;

	return BPrivate::Storage::Sniffer::parse(rule, &snifferRule, parseError);
}


// Guesses a MIME type for the entry referred to by the given
// entry_ref.
status_t
BMimeType::GuessMimeType(const entry_ref *file, BMimeType *type)
{
	status_t err = file && type ? B_OK : B_BAD_VALUE;

	BMessage msg(B_REG_MIME_SNIFF);
	BMessage reply;
	status_t result;
	const char *str;

	// Build and send the message, read the reply
	if (!err)
		err = msg.AddRef("file ref", file);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;
	if (!err)
		err = reply.FindString("mime type", &str);
	if (!err)
		err = type->SetTo(str);

	return err;
}


// Guesses a MIME type for the supplied chunk of data.
status_t
BMimeType::GuessMimeType(const void *buffer, int32 length, BMimeType *type)
{
	status_t err = buffer && type ? B_OK : B_BAD_VALUE;

	BMessage msg(B_REG_MIME_SNIFF);
	BMessage reply;
	status_t result;
	const char *str;

	// Build and send the message, read the reply
	if (!err)
		err = msg.AddData("data", B_RAW_TYPE, buffer, length);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;
	if (!err)
		err = reply.FindString("mime type", &str);
	if (!err)
		err = type->SetTo(str);

	return err;
}


// Guesses a MIME type for the given filename.
status_t
BMimeType::GuessMimeType(const char *filename, BMimeType *type)
{
	status_t err = filename && type ? B_OK : B_BAD_VALUE;

	BMessage msg(B_REG_MIME_SNIFF);
	BMessage reply;
	status_t result;
	const char *str;
	
	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("filename", filename);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;
	if (!err)
		err = reply.FindString("mime type", &str);
	if (!err)
		err = type->SetTo(str);

	return err;
}


// Starts monitoring the MIME database for a given target.
status_t
BMimeType::StartWatching(BMessenger target)
{
	BMessage msg(B_REG_MIME_START_WATCHING);
	BMessage reply;
	status_t result;
	status_t err;

	// Build and send the message, read the reply
	err = msg.AddMessenger("target", target);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;

	return err;
}


// Stops monitoring the MIME database for a given target
status_t
BMimeType::StopWatching(BMessenger target)
{
	BMessage msg(B_REG_MIME_STOP_WATCHING);
	BMessage reply;
	status_t result;
	status_t err;

	// Build and send the message, read the reply
	err = msg.AddMessenger("target", target);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;

	return err;
}

// Initializes this object to the supplied MIME type
status_t
BMimeType::SetType(const char *mimeType)
{
	return SetTo(mimeType);
}


void BMimeType::_ReservedMimeType1() {}
void BMimeType::_ReservedMimeType2() {}
void BMimeType::_ReservedMimeType3() {}


// assignment operator.
// Unimplemented
BMimeType&
BMimeType::operator=(const BMimeType &)
{
	return *this;	// not implemented
}


// copy constructor
// Unimplemented
BMimeType::BMimeType(const BMimeType &)
{
}


status_t
BMimeType::GetSupportedTypes(BMessage *types)
{
	status_t err = InitCheck();
	if (!err)
		err = get_supported_types(Type(), types);

	return err;
}


// Sets the list of MIME types supported by the MIME type
status_t
BMimeType::SetSupportedTypes(const BMessage *types, bool fullSync)
{
	status_t err = InitCheck();

	BMessage msg(types ? B_REG_MIME_SET_PARAM : B_REG_MIME_DELETE_PARAM);
	BMessage reply;
	status_t result;

	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("type", Type());
	if (!err)
		err = msg.AddInt32("which", B_REG_MIME_SUPPORTED_TYPES);
	if (!err && types)
		err = msg.AddMessage("types", types);
	if (!err)
		err = msg.AddBool("full sync", fullSync);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err)
		err = result;

	return err;
}


// Returns a list of mime types associated with the given file extension
status_t
BMimeType::GetAssociatedTypes(const char *extension, BMessage *types)
{
	status_t err = extension && types ? B_OK : B_BAD_VALUE;

	BMessage msg(B_REG_MIME_GET_ASSOCIATED_TYPES);
	BMessage &reply = *types;
	status_t result;

	// Build and send the message, read the reply
	if (!err)
		err = msg.AddString("extension", extension);
	if (!err)
		err = BRoster::Private().SendTo(&msg, &reply, true);
	if (!err)
		err = (status_t)(reply.what == B_REG_RESULT ? B_OK : B_BAD_REPLY);
	if (!err)
		err = reply.FindInt32("result", &result);
	if (!err) 
		err = result;

	return err;	
}
