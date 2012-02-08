/*
 * Copyright (c) 2001-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 *		Alex Wilson (yourpalal2@gmail.com)
 */

/*!	BArchivable mix-in class defines the archiving protocol.
	Also some global archiving functions.
*/


#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <syslog.h>
#include <typeinfo>
#include <vector>

#include <AppFileInfo.h>
#include <Archivable.h>
#include <Entry.h>
#include <List.h>
#include <OS.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include <binary_compatibility/Support.h>

#include "ArchivingManagers.h"


using std::string;
using std::vector;

using namespace BPrivate::Archiving;

const char* B_CLASS_FIELD = "class";
const char* B_ADD_ON_FIELD = "add_on";
const int32 FUNC_NAME_LEN = 1024;

// TODO: consider moving these to a separate module, and making them more
//	full-featured (e.g., taking NS::ClassName::Function(Param p) instead
//	of just NS::ClassName)


static status_t
demangle_class_name(const char* name, BString& out)
{
// TODO: add support for template classes
//	_find__t12basic_string3ZcZt18string_char_traits1ZcZt24__default_alloc_template2b0i0PCccUlUl

	out = "";

#if __GNUC__ >= 4
	if (name[0] == 'N')
		name++;
	int nameLen;
	bool first = true;
	while ((nameLen = strtoul(name, (char**)&name, 10))) {
		if (!first)
			out += "::";
		else
			first = false;
		out.Append(name, nameLen);
		name += nameLen;
	}
	if (first)
		return B_BAD_VALUE;

#else
	if (name[0] == 'Q') {
		// The name is in a namespace
		int namespaceCount = 0;
		name++;
		if (name[0] == '_') {
			// more than 10 namespaces deep
			if (!isdigit(*++name))
				return B_BAD_VALUE;

			namespaceCount = strtoul(name, (char**)&name, 10);
			if (name[0] != '_')
				return B_BAD_VALUE;
		} else
			namespaceCount = name[0] - '0';

		name++;

		for (int i = 0; i < namespaceCount - 1; i++) {
			if (!isdigit(name[0]))
				return B_BAD_VALUE;

			int nameLength = strtoul(name, (char**)&name, 10);
			out.Append(name, nameLength);
			out += "::";
			name += nameLength;
		}
	}

	int nameLength = strtoul(name, (char**)&name, 10);
	out.Append(name, nameLength);
#endif

	return B_OK;
}


static void
mangle_class_name(const char* name, BString& out)
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
	while (pos != string::npos) {
		pos = origName.find_first_of("::", oldpos);
		spacenames.push_back(string(origName, oldpos, pos - oldpos));
		pos = origName.find_first_not_of("::", pos);
		oldpos = pos;
		++count;
	}

	//	Now mangle it into this:
	//		9testthree8testfour9Testthree8Testfour
	//			(for __GNUC__ > 2)
	//			this isn't always the proper mangled class name, it should
	//			actually have an 'N' prefix and 'E' suffix if the name is
	//			in > 0 namespaces, but these would have to be removed in
	//			build_function_name() (the only place this function is called)
	//			so we don't add them.
	//	or this:
	//		Q49testthree8testfour9Testthree8Testfour
	//			(for __GNUC__ == 2)

	out = "";
#if __GNUC__ == 2
	if (count > 1) {
		out += 'Q';
		if (count > 10)
			out += '_';
		out << count;
		if (count > 10)
			out += '_';
	}
#endif

	for (unsigned int i = 0; i < spacenames.size(); ++i) {
		out << (int)spacenames[i].length();
		out += spacenames[i].c_str();
	}
}


static void
build_function_name(const BString& className, BString& funcName)
{
	funcName = "";

	//	This is what we're after:
	//		Instantiate__Q28OpenBeOS11BArchivableP8BMessage
	mangle_class_name(className.String(), funcName);
#if __GNUC__ >= 4
	funcName.Prepend("_ZN");
	funcName.Append("11InstantiateE");
#else
	funcName.Prepend("Instantiate__");
#endif
	funcName.Append("P8BMessage");
}


static bool
add_private_namespace(BString& name)
{
	if (name.Compare("_", 1) != 0)
		return false;

	name.Prepend("BPrivate::");
	return true;
}


static instantiation_func
find_function_in_image(BString& funcName, image_id id, status_t& err)
{
	instantiation_func instantiationFunc = NULL;
	err = get_image_symbol(id, funcName.String(), B_SYMBOL_TYPE_TEXT,
		(void**)&instantiationFunc);
	if (err != B_OK)
		return NULL;

	return instantiationFunc;
}


static status_t
check_signature(const char* signature, image_info& info)
{
	if (signature == NULL) {
		// If it wasn't specified, anything "matches"
		return B_OK;
	}

	// Get image signature
	BFile file(info.name, B_READ_ONLY);
	status_t err = file.InitCheck();
	if (err != B_OK)
		return err;

	char imageSignature[B_MIME_TYPE_LENGTH];
	BAppFileInfo appFileInfo(&file);
	err = appFileInfo.GetSignature(imageSignature);
	if (err != B_OK) {
		syslog(LOG_ERR, "instantiate_object - couldn't get mime sig for %s",
			info.name);
		return err;
	}

	if (strcmp(signature, imageSignature))
		return B_MISMATCHED_VALUES;

	return B_OK;
}


//	#pragma mark -


BArchivable::BArchivable()
	:
	fArchivingToken(NULL_TOKEN)
{
}


BArchivable::BArchivable(BMessage* from)
	:
	fArchivingToken(NULL_TOKEN)
{
	if (BUnarchiver::IsArchiveManaged(from)) {
		BUnarchiver::PrepareArchive(from);
		BUnarchiver(from).RegisterArchivable(this);
	}
}


BArchivable::~BArchivable()
{
}


status_t
BArchivable::Archive(BMessage* into, bool deep) const
{
	if (!into) {
		// TODO: logging/other error reporting?
		return B_BAD_VALUE;
	}

	if (BManagerBase::ArchiveManager(into))
		BArchiver(into).RegisterArchivable(this);

	BString name;
	status_t status = demangle_class_name(typeid(*this).name(), name);
	if (status != B_OK)
		return status;

	return into->AddString(B_CLASS_FIELD, name);
}


BArchivable*
BArchivable::Instantiate(BMessage* from)
{
	debugger("Can't create a plain BArchivable object");
	return NULL;
}


status_t
BArchivable::Perform(perform_code d, void* arg)
{
	switch (d) {
		case PERFORM_CODE_ALL_UNARCHIVED:
		{
			perform_data_all_unarchived* data =
				(perform_data_all_unarchived*)arg;

			data->return_value = BArchivable::AllUnarchived(data->archive);
			return B_OK;
		}

		case PERFORM_CODE_ALL_ARCHIVED:
		{
			perform_data_all_archived* data =
				(perform_data_all_archived*)arg;

			data->return_value = BArchivable::AllArchived(data->archive);
			return B_OK;
		}
	}
	return B_NAME_NOT_FOUND;
}


status_t
BArchivable::AllUnarchived(const BMessage* archive)
{
	return B_OK;
}


status_t
BArchivable::AllArchived(BMessage* archive) const
{
	return B_OK;
}


// #pragma mark -


BArchiver::BArchiver(BMessage* archive)
	:
	fManager(BManagerBase::ArchiveManager(archive)),
	fArchive(archive),
	fFinished(false)
{
	if (!fManager)
		fManager = new BArchiveManager(this);
}


BArchiver::~BArchiver()
{
	if (!fFinished)
		fManager->ArchiverLeaving(this, B_OK);
}


status_t
BArchiver::AddArchivable(const char* name, BArchivable* archivable, bool deep)
{
	int32 token;
	status_t err = GetTokenForArchivable(archivable, deep, token);

	if (err != B_OK)
		return err;

	return fArchive->AddInt32(name, token);
}


status_t
BArchiver::GetTokenForArchivable(BArchivable* archivable,
	bool deep, int32& _token)
{
	return fManager->ArchiveObject(archivable, deep, _token);
}


bool
BArchiver::IsArchived(BArchivable* archivable)
{
	return fManager->IsArchived(archivable);
}


status_t
BArchiver::Finish(status_t err)
{
	if (fFinished)
		debugger("Finish() called multiple times on same BArchiver.");

	fFinished = true;

	return fManager->ArchiverLeaving(this, err);
}


BMessage*
BArchiver::ArchiveMessage() const
{
	return fArchive;
}


void
BArchiver::RegisterArchivable(const BArchivable* archivable)
{
	fManager->RegisterArchivable(archivable);
}


// #pragma mark -


BUnarchiver::BUnarchiver(const BMessage* archive)
	:
	fManager(BManagerBase::UnarchiveManager(archive)),
	fArchive(archive),
	fFinished(false)
{
}


BUnarchiver::~BUnarchiver()
{
	if (!fFinished && fManager)
		fManager->UnarchiverLeaving(this, B_OK);
}


template<>
status_t
BUnarchiver::GetObject<BArchivable>(int32 token,
	ownership_policy owning, BArchivable*& object)
{
	_CallDebuggerIfManagerNull();
	return fManager->GetArchivableForToken(token, owning, object);
}


template<>
status_t
BUnarchiver::FindObject<BArchivable>(const char* name,
	int32 index, ownership_policy owning, BArchivable*& archivable)
{
	archivable = NULL;
	int32 token;
	status_t err = fArchive->FindInt32(name, index, &token);
	if (err != B_OK)
		return err;

	return GetObject(token, owning, archivable);
}


bool
BUnarchiver::IsInstantiated(int32 token)
{
	_CallDebuggerIfManagerNull();
	return fManager->IsInstantiated(token);
}


bool
BUnarchiver::IsInstantiated(const char* field, int32 index)
{
	int32 token;
	if (fArchive->FindInt32(field, index, &token) == B_OK)
		return IsInstantiated(token);
	return false;
}


status_t
BUnarchiver::Finish(status_t err)
{
	if (fFinished)
		debugger("Finish() called multiple times on same BArchiver.");

	fFinished = true;
	if (fManager)
		return fManager->UnarchiverLeaving(this, err);
	else
		return B_OK;
}


const BMessage*
BUnarchiver::ArchiveMessage() const
{
	return fArchive;
}


void
BUnarchiver::AssumeOwnership(BArchivable* archivable)
{
	_CallDebuggerIfManagerNull();
	fManager->AssumeOwnership(archivable);
}


void
BUnarchiver::RelinquishOwnership(BArchivable* archivable)
{
	_CallDebuggerIfManagerNull();
	fManager->RelinquishOwnership(archivable);
}


bool
BUnarchiver::IsArchiveManaged(const BMessage* archive)
{
	// managed child archives will return here
	if (BManagerBase::ManagerPointer(archive))
		return true;

	if (!archive)
		return false;

	// managed top level archives return here
	bool dummy;
	if (archive->FindBool(kManagedField, &dummy) == B_OK)
		return true;

	return false;
}


template<>
status_t
BUnarchiver::InstantiateObject<BArchivable>(BMessage* from,
	BArchivable*& object)
{
	BUnarchiver unarchiver(BUnarchiver::PrepareArchive(from));
	object = instantiate_object(from);
	return unarchiver.Finish();
}


BMessage*
BUnarchiver::PrepareArchive(BMessage*& archive)
{
	// this check allows PrepareArchive to be
	// called on new or old-style archives
	if (BUnarchiver::IsArchiveManaged(archive)) {
		BUnarchiveManager* manager = BManagerBase::UnarchiveManager(archive);
		if (!manager)
			manager = new BUnarchiveManager(archive);
		manager->Acquire();
	}
	return archive;
}


void
BUnarchiver::RegisterArchivable(BArchivable* archivable)
{
	_CallDebuggerIfManagerNull();
	fManager->RegisterArchivable(archivable);
}


void
BUnarchiver::_CallDebuggerIfManagerNull()
{
	if (!fManager)
		debugger("BUnarchiver used with legacy or unprepared archive.");
}


// #pragma mark -


BArchivable*
instantiate_object(BMessage* archive, image_id* _id)
{
	status_t statusBuffer;
	status_t* status = &statusBuffer;
	if (_id != NULL)
		status = _id;

	// Check our params
	if (archive == NULL) {
		syslog(LOG_ERR, "instantiate_object failed: NULL BMessage argument");
		*status = B_BAD_VALUE;
		return NULL;
	}

	// Get class name from archive
	const char* className = NULL;
	status_t err = archive->FindString(B_CLASS_FIELD, &className);
	if (err) {
		syslog(LOG_ERR, "instantiate_object failed: Failed to find an entry "
			"defining the class name (%s).", strerror(err));
		*status = B_BAD_VALUE;
		return NULL;
	}

	// Get sig from archive
	const char* signature = NULL;
	bool hasSignature = archive->FindString(B_ADD_ON_FIELD, &signature) == B_OK;

	instantiation_func instantiationFunc = find_instantiation_func(className,
		signature);

	// if find_instantiation_func() can't locate Class::Instantiate()
	// and a signature was specified
	if (!instantiationFunc && hasSignature) {
		// use BRoster::FindApp() to locate an app or add-on with the symbol
		BRoster Roster;
		entry_ref ref;
		err = Roster.FindApp(signature, &ref);

		// if an entry_ref is obtained
		BEntry entry;
		if (err == B_OK)
			err = entry.SetTo(&ref);

		BPath path;
		if (err == B_OK)
			err = entry.GetPath(&path);

		if (err != B_OK) {
			syslog(LOG_ERR, "instantiate_object failed: Error finding app "
				"with signature \"%s\" (%s)", signature, strerror(err));
			*status = err;
			return NULL;
		}

		// load the app/add-on
		image_id addOn = load_add_on(path.Path());
		if (addOn < B_OK) {
			syslog(LOG_ERR, "instantiate_object failed: Could not load "
				"add-on %s: %s.", path.Path(), strerror(addOn));
			*status = addOn;
			return NULL;
		}

		// Save the image_id
		if (_id != NULL)
			*_id = addOn;

		BString name = className;
		for (int32 pass = 0; pass < 2; pass++) {
			BString funcName;
			build_function_name(name, funcName);

			instantiationFunc = find_function_in_image(funcName, addOn, err);
			if (instantiationFunc != NULL)
				break;

			// Check if we have a private class, and add the BPrivate namespace
			// (for backwards compatibility)
			if (!add_private_namespace(name))
				break;
		}

		if (instantiationFunc == NULL) {
			syslog(LOG_ERR, "instantiate_object failed: Failed to find exported "
				"Instantiate static function for class %s.", className);
			*status = B_NAME_NOT_FOUND;
			return NULL;
		}
	} else if (instantiationFunc == NULL) {
		syslog(LOG_ERR, "instantiate_object failed: No signature specified "
			"in archive, looking for class \"%s\".", className);
		*status = B_NAME_NOT_FOUND;
		return NULL;
	}

	// if Class::Instantiate(BMessage*) was found
	if (instantiationFunc != NULL) {
		// use to create and return an object instance
		return instantiationFunc(archive);
	}

	return NULL;
}


BArchivable*
instantiate_object(BMessage* from)
{
	return instantiate_object(from, NULL);
}


bool
validate_instantiation(BMessage* from, const char* className)
{
	// Make sure our params are kosher -- original skimped here =P
	if (!from) {
		errno = B_BAD_VALUE;
		return false;
	}

	BString name = className;
	for (int32 pass = 0; pass < 2; pass++) {
		const char* archiveClassName;
		for (int32 index = 0; from->FindString(B_CLASS_FIELD, index,
				&archiveClassName) == B_OK; ++index) {
			if (name == archiveClassName)
				return true;
		}

		if (!add_private_namespace(name))
			break;
	}

	errno = B_MISMATCHED_VALUES;
	syslog(LOG_ERR, "validate_instantiation failed on class %s.", className);

	return false;
}


instantiation_func
find_instantiation_func(const char* className, const char* signature)
{
	if (className == NULL) {
		errno = B_BAD_VALUE;
		return NULL;
	}

	thread_info threadInfo;
	status_t err = get_thread_info(find_thread(NULL), &threadInfo);
	if (err != B_OK) {
		errno = err;
		return NULL;
	}

	instantiation_func instantiationFunc = NULL;
	image_info imageInfo;

	BString name = className;
	for (int32 pass = 0; pass < 2; pass++) {
		BString funcName;
		build_function_name(name, funcName);

		// for each image_id in team_id
		int32 cookie = 0;
		while (instantiationFunc == NULL
			&& get_next_image_info(threadInfo.team, &cookie, &imageInfo)
				== B_OK) {
			instantiationFunc = find_function_in_image(funcName, imageInfo.id,
				err);
		}
		if (instantiationFunc != NULL)
			break;

		// Check if we have a private class, and add the BPrivate namespace
		// (for backwards compatibility)
		if (!add_private_namespace(name))
			break;
	}

	if (instantiationFunc != NULL
		&& check_signature(signature, imageInfo) != B_OK)
		return NULL;

	return instantiationFunc;
}


instantiation_func
find_instantiation_func(const char* className)
{
	return find_instantiation_func(className, NULL);
}


instantiation_func
find_instantiation_func(BMessage* archive)
{
	if (archive == NULL) {
		errno = B_BAD_VALUE;
		return NULL;
	}

	const char* name = NULL;
	const char* signature = NULL;
	if (archive->FindString(B_CLASS_FIELD, &name) != B_OK
		|| archive->FindString(B_ADD_ON_FIELD, &signature)) {
		errno = B_BAD_VALUE;
		return NULL;
	}

	return find_instantiation_func(name, signature);
}


// BArchivable binary compatibility
#if __GNUC__ == 2


extern "C" status_t
_ReservedArchivable1__11BArchivable(BArchivable* archivable,
	const BMessage* archive)
{
	// AllUnarchived
	perform_data_all_unarchived performData;
	performData.archive = archive;

	archivable->Perform(PERFORM_CODE_ALL_UNARCHIVED, &performData);
	return performData.return_value;
}


extern "C" status_t
_ReservedArchivable2__11BArchivable(BArchivable* archivable,
	BMessage* archive)
{
	// AllArchived
	perform_data_all_archived performData;
	performData.archive = archive;

	archivable->Perform(PERFORM_CODE_ALL_ARCHIVED, &performData);
	return performData.return_value;
}


#elif __GNUC__ > 2


extern "C" status_t
_ZN11BArchivable20_ReservedArchivable1Ev(BArchivable* archivable,
	const BMessage* archive)
{
	// AllUnarchived
	perform_data_all_unarchived performData;
	performData.archive = archive;

	archivable->Perform(PERFORM_CODE_ALL_UNARCHIVED, &performData);
	return performData.return_value;
}


extern "C" status_t
_ZN11BArchivable20_ReservedArchivable2Ev(BArchivable* archivable,
	BMessage* archive)
{
	// AllArchived
	perform_data_all_archived performData;
	performData.archive = archive;

	archivable->Perform(PERFORM_CODE_ALL_ARCHIVED, &performData);
	return performData.return_value;
}


#endif // _GNUC__ > 2


void BArchivable::_ReservedArchivable3() {}


