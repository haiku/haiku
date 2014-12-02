/*
 * Public domain source code.
 *
 * Author:
 *		Joseph "looncraz" Groover <looncraz@satx.rr.com>
 */


#include <DecorInfo.h>

#include <new>
#include <stdio.h>

#include <Autolock.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Resources.h>
#include <SystemCatalog.h>

#include <DecoratorPrivate.h>


#define B_TRANSLATION_CONTEXT "Default decorator about box"


namespace BPrivate {


DecorInfo::DecorInfo()
	:
	fVersion(0),
	fModificationTime(0),
	fInitStatus(B_NO_INIT)
{
}


DecorInfo::DecorInfo(const BString& path)
	:
	fPath(path),
	fVersion(0),
	fModificationTime(0),
	fInitStatus(B_NO_INIT)
{
	BEntry entry(path.String(), true);
	entry.GetRef(&fRef);

	_Init();
}


DecorInfo::DecorInfo(const entry_ref& ref)
	:
	fRef(ref),
	fVersion(0),
	fModificationTime(0),
	fInitStatus(B_NO_INIT)
{
	BPath path(&ref);
	fPath = path.Path();

	_Init();
}


DecorInfo::~DecorInfo()
{
}


status_t
DecorInfo::SetTo(const entry_ref& ref)
{
	Unset();

	BPath path(&ref);
	fPath = path.Path();
	fRef = ref;
	_Init();

	return InitCheck();
}


status_t
DecorInfo::SetTo(BString path)
{
	BEntry entry(path.String(), true);
	entry_ref ref;
	entry.GetRef(&ref);
	return SetTo(ref);
}


status_t
DecorInfo::InitCheck()	const
{
	return fInitStatus;
}


void
DecorInfo::Unset()
{
	fRef = entry_ref();
	fPath = "";
	fName = "";
	fAuthors = "";
	fShortDescription = "";
	fLicenseURL = "";
	fLicenseName = "";
	fSupportURL = "";
	fVersion = 0;
	fModificationTime = 0;
	fInitStatus = B_NO_INIT;
}


bool
DecorInfo::IsDefault() const
{
	return fInitStatus == B_OK && fPath == "Default";
}


BString
DecorInfo::Path() const
{
	return fPath;
}


const entry_ref*
DecorInfo::Ref() const
{
	if (InitCheck() != B_OK || IsDefault())
		return NULL;
	return &fRef;
}


BString
DecorInfo::Name() const
{
	return fName;
}


BString
DecorInfo::ShortcutName() const
{
	if (Ref())
		return fRef.name;
	return Name();
}


BString
DecorInfo::Authors() const
{
	return fAuthors;
}


BString
DecorInfo::ShortDescription() const
{
	return fShortDescription;
}


BString
DecorInfo::LongDescription() const
{
	return fLongDescription;
}


BString
DecorInfo::LicenseURL() const
{
	return fLicenseURL;
}


BString
DecorInfo::LicenseName() const
{
	return fLicenseName;
}


BString
DecorInfo::SupportURL() const
{
	return fSupportURL;
}


float
DecorInfo::Version() const
{
	return fVersion;
}


time_t
DecorInfo::ModificationTime() const
{
	return fModificationTime;
}


bool
DecorInfo::CheckForChanges(bool& deleted)
{
	if (InitCheck() != B_OK)
		return false;

	BEntry entry(&fRef);

	if (entry.InitCheck() != B_OK)
		return false;

	if (!entry.Exists()) {
		deleted = true;
		return true;
	}

	time_t modtime = 0;
	if (entry.GetModificationTime(&modtime) != B_OK) {
		fprintf(stderr, "DecorInfo::CheckForChanges()\tERROR: "
			"BEntry:GetModificationTime() failed\n");
		return false;
	}

	if (fModificationTime != modtime) {
		_Init(true);
		return true;
	}

	return false;
}


void
DecorInfo::_Init(bool isUpdate)
{
	if (!isUpdate && InitCheck() != B_NO_INIT) {
		// TODO: remove after validation
		fprintf(stderr, "DecorInfo::_Init()\tImproper init state\n");
		return;
	}

	BEntry entry;

	if (fPath == "Default") {
		if (isUpdate) {
			// should never happen
			fprintf(stderr, "DecorInfo::_Init(true)\tBUG BUG updating default"
				"decorator!?!?!\n");
			return;
		}

		fAuthors = "DarkWyrm, Stephan Aßmus, Clemens Zeidler, Ingo Weinhold";
		fLongDescription = fShortDescription;
		fLicenseURL = "http://";
		fLicenseName = "MIT";
		fSupportURL = "http://www.haiku-os.org/";
		fVersion = 0.5;
		fInitStatus = B_OK;

		fName = gSystemCatalog.GetString(B_TRANSLATE_MARK("Default"),
			B_TRANSLATION_CONTEXT);
		fShortDescription = gSystemCatalog.GetString(B_TRANSLATE_MARK(
				"Default Haiku window decorator."),
			B_TRANSLATION_CONTEXT);

		// The following is to get the modification time of the app_server
		// and, thusly, the Default decorator...
		// If you can make it more simple, please do!
		BPath path;
		find_directory(B_SYSTEM_SERVERS_DIRECTORY, &path);
		path.Append("app_server");
		entry.SetTo(path.Path(), true);
		if (!entry.Exists()) {
			fprintf(stderr, "Server MIA the world has become its slave! "
				"Call the CIA!\n");
			return;
		}

		entry.GetModificationTime(&fModificationTime);
		return;
	}

	// Is a file system object...

	entry.SetTo(&fRef, true);	// follow link
	if (entry.InitCheck() != B_OK) {
		fInitStatus = entry.InitCheck();
		return;
	}

	if (!entry.Exists()) {
		if (isUpdate) {
			fprintf(stderr, "DecorInfo::_Init()\tERROR: decorator deleted"
					" after CheckForChanges() found it!\n");
			fprintf(stderr, "DecorInfo::_Init()\tERROR: DecorInfo will "
					"Unset\n");
			Unset();
		}
		return;
	}

	// update fRef to match file system object
	entry.GetRef(&fRef);
	entry.GetModificationTime(&fModificationTime);

	BResources resources(&fRef);
	if (resources.InitCheck() != B_OK) {
		fprintf(stderr, "DecorInfo::_Init()\t BResource InitCheck() failure\n");
		return;
	}

	size_t infoSize = 0;
	const void* infoData = resources.LoadResource(B_MESSAGE_TYPE,
		"be:decor:info", &infoSize);
	BMessage infoMessage;

	if (infoData == NULL || infoSize == 0
		|| infoMessage.Unflatten((const char*)infoData) != B_OK) {
		fprintf(stderr, "DecorInfo::_init()\tNo extended information found for"
			" \"%s\"\n", fRef.name);
	} else {
		infoMessage.FindString("name", &fName);
		infoMessage.FindString("authors", &fAuthors);
		infoMessage.FindString("short_descr", &fShortDescription);
		infoMessage.FindString("long_descr", &fLongDescription);
		infoMessage.FindString("lic_url", &fLicenseURL);
		infoMessage.FindString("lic_name", &fLicenseName);
		infoMessage.FindString("support_url", &fSupportURL);
		infoMessage.FindFloat ("version", &fVersion);
	}

	fInitStatus = B_OK;
	fName = fRef.name;
}


// #pragma mark - DecorInfoUtility


DecorInfoUtility::DecorInfoUtility(bool scanNow)
	:
	fHasScanned(false)
{
	// get default decorator from app_server
	DecorInfo* info = new(std::nothrow) DecorInfo("Default");
	if (info == NULL || info->InitCheck() != B_OK)	{
		delete info;
		fprintf(stderr, "DecorInfoUtility::constructor\tdefault decorator's "
			"DecorInfo failed InitCheck()\n");
		return;
	}

	fList.AddItem(info);

	if (scanNow)
		ScanDecorators();
}


DecorInfoUtility::~DecorInfoUtility()
{
	BAutolock _(fLock);
	for	(int i = fList.CountItems() - 1; i >= 0; --i)
		delete fList.ItemAt(i);
}


status_t
DecorInfoUtility::ScanDecorators()
{
	status_t result;

	BPath systemPath;
	result = find_directory(B_SYSTEM_ADDONS_DIRECTORY, &systemPath);
	if (result == B_OK)
		result = systemPath.Append("decorators");

	if (result == B_OK) {
		BDirectory systemDirectory(systemPath.Path());
		result = systemDirectory.InitCheck();
		if (result == B_OK) {
			result = _ScanDecorators(systemDirectory);
			if (result != B_OK) {
				fprintf(stderr, "DecorInfoUtility::ScanDecorators()\tERROR: %s\n",
					strerror(result));
				return result;
			}
		}
	}

	BPath systemNonPackagedPath;
	result = find_directory(B_SYSTEM_NONPACKAGED_ADDONS_DIRECTORY,
		&systemNonPackagedPath);
	if (result == B_OK)
		result = systemNonPackagedPath.Append("decorators");

	if (result == B_OK) {
		BDirectory systemNonPackagedDirectory(systemNonPackagedPath.Path());
		result = systemNonPackagedDirectory.InitCheck();
		if (result == B_OK) {
			result = _ScanDecorators(systemNonPackagedDirectory);
			if (result != B_OK) {
				fprintf(stderr, "DecorInfoUtility::ScanDecorators()\tERROR: %s\n",
					strerror(result));
				return result;
			}
		}
	}

	BPath userPath;
	result = find_directory(B_USER_ADDONS_DIRECTORY, &userPath);
	if (result == B_OK)
		result = userPath.Append("decorators");

	if (result == B_OK) {
		BDirectory userDirectory(userPath.Path());
		result = userDirectory.InitCheck();
		if (result == B_OK) {
			result = _ScanDecorators(userDirectory);
			if (result != B_OK) {
				fprintf(stderr, "DecorInfoUtility::ScanDecorators()\tERROR: %s\n",
					strerror(result));
				return result;
			}
		}
	}

	BPath userNonPackagedPath;
	result = find_directory(B_USER_NONPACKAGED_ADDONS_DIRECTORY,
		&userNonPackagedPath);
	if (result == B_OK)
		result = userNonPackagedPath.Append("decorators");

	if (result == B_OK) {
		BDirectory userNonPackagedDirectory(userNonPackagedPath.Path());
		result = userNonPackagedDirectory.InitCheck();
		if (result == B_OK) {
			result = _ScanDecorators(userNonPackagedDirectory);
			if (result != B_OK) {
				fprintf(stderr, "DecorInfoUtility::ScanDecorators()\tERROR: %s\n",
					strerror(result));
				return result;
			}
		}
	}

	fHasScanned = true;

	return B_OK;
}


int32
DecorInfoUtility::CountDecorators()
{
	BAutolock _(fLock);
	if (!fHasScanned)
		ScanDecorators();

	return fList.CountItems();
}


DecorInfo*
DecorInfoUtility::DecoratorAt(int32 index)
{
	BAutolock _(fLock);
	return fList.ItemAt(index);
}


DecorInfo*
DecorInfoUtility::FindDecorator(const BString& string)
{
	if (string.Length() == 0)
		return CurrentDecorator();

	if (string.ICompare("default") == 0)
		return DefaultDecorator();

	BAutolock _(fLock);
	if (!fHasScanned)
		ScanDecorators();

	// search by path
	DecorInfo* decor = _FindDecor(string);
	if (decor != NULL)
		return decor;

	// search by name or short cut name
	for (int i = 1; i < fList.CountItems(); ++i) {
		decor = fList.ItemAt(i);
		if (string.ICompare(decor->ShortcutName()) == 0
			|| string.ICompare(decor->Name()) == 0) {
			return decor;
		}
	}

	return NULL;
}


DecorInfo*
DecorInfoUtility::CurrentDecorator()
{
	BAutolock _(fLock);
	if (!fHasScanned)
		ScanDecorators();

	BString name;
	get_decorator(name);
	return FindDecorator(name);
}


DecorInfo*
DecorInfoUtility::DefaultDecorator()
{
	BAutolock _(fLock);
	return fList.ItemAt(0);
}


bool
DecorInfoUtility::IsCurrentDecorator(DecorInfo* decor)
{
	BAutolock _(fLock);
	if (decor == NULL)
		 return false;
	return decor->Path() == CurrentDecorator()->Path();
}


status_t
DecorInfoUtility::SetDecorator(DecorInfo* decor)
{
	if (decor == NULL)
		return B_BAD_VALUE;

	BAutolock _(fLock);
	if (decor->IsDefault())
		return set_decorator("Default");

	return set_decorator(decor->Path());
}


status_t
DecorInfoUtility::SetDecorator(int32 index)
{
	BAutolock _(fLock);
	if (!fHasScanned)
		return B_ERROR;

	DecorInfo* decor = DecoratorAt(index);
	if (decor == NULL)
		return B_BAD_INDEX;

	return SetDecorator(decor);
}


status_t
DecorInfoUtility::Preview(DecorInfo* decor, BWindow* window)
{
	if (decor == NULL)
		return B_BAD_VALUE;

	return preview_decorator(decor->Path(), window);
}


// #pargma mark - private


DecorInfo*
DecorInfoUtility::_FindDecor(const BString& pathString)
{
	// find decor by path and path alone!
	if (!fLock.IsLocked()) {
		fprintf(stderr, "DecorInfoUtility::_find_decor()\tfailure to lock! - "
			"BUG BUG BUG\n");
		return NULL;
	}

	if (pathString == "Default")
		return fList.ItemAt(0);

	for (int i = 1; i < fList.CountItems(); ++i) {
		DecorInfo* decor = fList.ItemAt(i);
		// Find the DecoratorInfo either by its true current location or by
		// what we still think the location is (before we had a chance to
		// update). NOTE: This will only catch the case when the user moved the
		// folder in which the add-on file lives. It will not work when the user
		// moves the add-on file itself or renames it.
		BPath path(decor->Ref());
		if (path.Path() == pathString || decor->Path() == pathString)
			return decor;
	}

	return NULL;
}


status_t
DecorInfoUtility::_ScanDecorators(BDirectory decoratorDirectory)
{
	BAutolock _(fLock);

	// First, run through our list and DecorInfos CheckForChanges()
	if (fHasScanned) {
		for (int i = fList.CountItems() - 1; i > 0; --i) {
			DecorInfo* decorInfo = fList.ItemAt(i);

			bool deleted = false;
			decorInfo->CheckForChanges(deleted);

			if (deleted) {
				fList.RemoveItem(decorInfo);
				delete decorInfo;
			}
		}
	}

	entry_ref ref;
	// Now, look at file system, skip the entries for which we already have
	// a DecorInfo in the list.
	while (decoratorDirectory.GetNextRef(&ref) == B_OK) {
		BPath path(&decoratorDirectory);
		status_t result = path.Append(ref.name);
		if (result != B_OK) {
			fprintf(stderr, "DecorInfoUtility::_ScanDecorators()\tFailed to"
				"append decorator file to path, skipping: %s.\n", strerror(result));
			continue;
		}
		if (_FindDecor(path.Path()) != NULL)
			continue;

		DecorInfo* decorInfo = new(std::nothrow) DecorInfo(ref);
		if (decorInfo == NULL || decorInfo->InitCheck() != B_OK) {
			fprintf(stderr, "DecorInfoUtility::_ScanDecorators()\tInitCheck() "
				"failure on decorator, skipping.\n");
			delete decorInfo;
			continue;
		}

		fList.AddItem(decorInfo);
	}

	return B_OK;
}


}	// namespace BPrivate
