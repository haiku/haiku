/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/PackageInfo.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new>

#include <File.h>
#include <Entry.h>
#include <Message.h>
#include <package/hpkg/NoErrorOutput.h>
#include <package/hpkg/PackageReader.h>
#include <package/hpkg/v1/PackageInfoContentHandler.h>
#include <package/hpkg/v1/PackageReader.h>
#include <package/PackageInfoContentHandler.h>

#include "PackageInfoParser.h"
#include "PackageInfoStringBuilder.h"


namespace BPackageKit {


const char* const BPackageInfo::kElementNames[B_PACKAGE_INFO_ENUM_COUNT] = {
	"name",
	"summary",
	"description",
	"vendor",
	"packager",
	"architecture",
	"version",
	"copyrights",
	"licenses",
	"provides",
	"requires",
	"supplements",
	"conflicts",
	"freshens",
	"replaces",
	"flags",
	"urls",
	"source-urls",
	"checksum",		// not being parsed, computed externally
	NULL,			// install-path -- not settable via .PackageInfo
	"base-package",
	"global-writable-files",
	"user-settings-files",
	"users",
	"groups",
	"post-install-scripts"
};


const char* const
BPackageInfo::kArchitectureNames[B_PACKAGE_ARCHITECTURE_ENUM_COUNT] = {
	"any",
	"x86",
	"x86_gcc2",
	"source",
	"x86_64",
	"ppc",
	"arm",
	"m68k",
	"sparc",
	"arm64",
	"riscv64"
};


const char* const BPackageInfo::kWritableFileUpdateTypes[
		B_WRITABLE_FILE_UPDATE_TYPE_ENUM_COUNT] = {
	"keep-old",
	"manual",
	"auto-merge",
};


// #pragma mark - FieldName


struct BPackageInfo::FieldName {
	FieldName(const char* prefix, const char* suffix)
	{
		size_t prefixLength = strlen(prefix);
		size_t suffixLength = strlen(suffix);
		if (prefixLength + suffixLength >= sizeof(fFieldName)) {
			fFieldName[0] = '\0';
			return;
		}

		memcpy(fFieldName, prefix, prefixLength);
		memcpy(fFieldName + prefixLength, suffix, suffixLength);
		fFieldName[prefixLength + suffixLength] = '\0';
	}

	bool ReplaceSuffix(size_t prefixLength, const char* suffix)
	{
		size_t suffixLength = strlen(suffix);
		if (prefixLength + suffixLength >= sizeof(fFieldName)) {
			fFieldName[0] = '\0';
			return false;
		}

		memcpy(fFieldName + prefixLength, suffix, suffixLength);
		fFieldName[prefixLength + suffixLength] = '\0';
		return true;
	}

	bool IsValid() const
	{
		return fFieldName[0] != '\0';
	}

	operator const char*()
	{
		return fFieldName;
	}

private:
	char	fFieldName[64];
};


// #pragma mark - PackageFileLocation


struct BPackageInfo::PackageFileLocation {
	PackageFileLocation(const char* path)
		:
		fPath(path),
		fFD(-1)
	{
	}

	PackageFileLocation(int fd)
		:
		fPath(NULL),
		fFD(fd)
	{
	}

	const char* Path() const
	{
		return fPath;
	}

	int FD() const
	{
		return fFD;
	}

private:
	const char*	fPath;
	int			fFD;
};


// #pragma mark - BPackageInfo


BPackageInfo::BPackageInfo()
	:
	BArchivable(),
	fFlags(0),
	fArchitecture(B_PACKAGE_ARCHITECTURE_ENUM_COUNT),
	fCopyrightList(4),
	fLicenseList(4),
	fURLList(4),
	fSourceURLList(4),
	fGlobalWritableFileInfos(4, true),
	fUserSettingsFileInfos(4, true),
	fUsers(4, true),
	fGroups(4),
	fPostInstallScripts(4),
	fProvidesList(20, true),
	fRequiresList(20, true),
	fSupplementsList(20, true),
	fConflictsList(4, true),
	fFreshensList(4, true),
	fReplacesList(4)
{
}


BPackageInfo::BPackageInfo(BMessage* archive, status_t* _error)
	:
	BArchivable(archive),
	fFlags(0),
	fArchitecture(B_PACKAGE_ARCHITECTURE_ENUM_COUNT),
	fCopyrightList(4),
	fLicenseList(4),
	fURLList(4),
	fSourceURLList(4),
	fGlobalWritableFileInfos(4, true),
	fUserSettingsFileInfos(4, true),
	fUsers(4, true),
	fGroups(4),
	fPostInstallScripts(4),
	fProvidesList(20, true),
	fRequiresList(20, true),
	fSupplementsList(20, true),
	fConflictsList(4, true),
	fFreshensList(4, true),
	fReplacesList(4)
{
	status_t error;
	int32 architecture;
	if ((error = archive->FindString("name", &fName)) == B_OK
		&& (error = archive->FindString("summary", &fSummary)) == B_OK
		&& (error = archive->FindString("description", &fDescription)) == B_OK
		&& (error = archive->FindString("vendor", &fVendor)) == B_OK
		&& (error = archive->FindString("packager", &fPackager)) == B_OK
		&& (error = archive->FindString("basePackage", &fBasePackage)) == B_OK
		&& (error = archive->FindUInt32("flags", &fFlags)) == B_OK
		&& (error = archive->FindInt32("architecture", &architecture)) == B_OK
		&& (error = _ExtractVersion(archive, "version", 0, fVersion)) == B_OK
		&& (error = _ExtractStringList(archive, "copyrights", fCopyrightList))
			== B_OK
		&& (error = _ExtractStringList(archive, "licenses", fLicenseList))
			== B_OK
		&& (error = _ExtractStringList(archive, "urls", fURLList)) == B_OK
		&& (error = _ExtractStringList(archive, "source-urls", fSourceURLList))
			== B_OK
		&& (error = _ExtractGlobalWritableFileInfos(archive,
			"global-writable-files", fGlobalWritableFileInfos)) == B_OK
		&& (error = _ExtractUserSettingsFileInfos(archive, "user-settings-files",
			fUserSettingsFileInfos)) == B_OK
		&& (error = _ExtractUsers(archive, "users", fUsers)) == B_OK
		&& (error = _ExtractStringList(archive, "groups", fGroups)) == B_OK
		&& (error = _ExtractStringList(archive, "post-install-scripts",
			fPostInstallScripts)) == B_OK
		&& (error = _ExtractResolvables(archive, "provides", fProvidesList))
			== B_OK
		&& (error = _ExtractResolvableExpressions(archive, "requires",
			fRequiresList)) == B_OK
		&& (error = _ExtractResolvableExpressions(archive, "supplements",
			fSupplementsList)) == B_OK
		&& (error = _ExtractResolvableExpressions(archive, "conflicts",
			fConflictsList)) == B_OK
		&& (error = _ExtractResolvableExpressions(archive, "freshens",
			fFreshensList)) == B_OK
		&& (error = _ExtractStringList(archive, "replaces", fReplacesList))
			== B_OK
		&& (error = archive->FindString("checksum", &fChecksum)) == B_OK
		&& (error = archive->FindString("install-path", &fInstallPath)) == B_OK
		&& (error = archive->FindString("file-name", &fFileName)) == B_OK) {
		if (architecture >= 0
			&& architecture <= B_PACKAGE_ARCHITECTURE_ENUM_COUNT) {
			fArchitecture = (BPackageArchitecture)architecture;
		} else
			error = B_BAD_DATA;
	}

	if (_error != NULL)
		*_error = error;
}


BPackageInfo::~BPackageInfo()
{
}


status_t
BPackageInfo::ReadFromConfigFile(const BEntry& packageInfoEntry,
	ParseErrorListener* listener)
{
	status_t result = packageInfoEntry.InitCheck();
	if (result != B_OK)
		return result;

	BFile file(&packageInfoEntry, B_READ_ONLY);
	if ((result = file.InitCheck()) != B_OK)
		return result;

	return ReadFromConfigFile(file, listener);
}


status_t
BPackageInfo::ReadFromConfigFile(BFile& packageInfoFile,
	ParseErrorListener* listener)
{
	off_t size;
	status_t result = packageInfoFile.GetSize(&size);
	if (result != B_OK)
		return result;

	BString packageInfoString;
	char* buffer = packageInfoString.LockBuffer(size);
	if (buffer == NULL)
		return B_NO_MEMORY;

	if ((result = packageInfoFile.Read(buffer, size)) < size) {
		packageInfoString.UnlockBuffer(0);
		return result >= 0 ? B_IO_ERROR : result;
	}

	buffer[size] = '\0';
	packageInfoString.UnlockBuffer(size);

	return ReadFromConfigString(packageInfoString, listener);
}


status_t
BPackageInfo::ReadFromConfigString(const BString& packageInfoString,
	ParseErrorListener* listener)
{
	Clear();

	Parser parser(listener);
	return parser.Parse(packageInfoString, this);
}


status_t
BPackageInfo::ReadFromPackageFile(const char* path)
{
	return _ReadFromPackageFile(PackageFileLocation(path));
}


status_t
BPackageInfo::ReadFromPackageFile(int fd)
{
	return _ReadFromPackageFile(PackageFileLocation(fd));
}


status_t
BPackageInfo::InitCheck() const
{
	if (fName.Length() == 0 || fSummary.Length() == 0
		|| fDescription.Length() == 0 || fVendor.Length() == 0
		|| fPackager.Length() == 0
		|| fArchitecture == B_PACKAGE_ARCHITECTURE_ENUM_COUNT
		|| fVersion.InitCheck() != B_OK
		|| fCopyrightList.IsEmpty() || fLicenseList.IsEmpty()
		|| fProvidesList.IsEmpty())
		return B_NO_INIT;

	// check global writable files
	int32 globalWritableFileCount = fGlobalWritableFileInfos.CountItems();
	for (int32 i = 0; i < globalWritableFileCount; i++) {
		const BGlobalWritableFileInfo* info
			= fGlobalWritableFileInfos.ItemAt(i);
		status_t error = info->InitCheck();
		if (error != B_OK)
			return error;
	}

	// check user settings files
	int32 userSettingsFileCount = fUserSettingsFileInfos.CountItems();
	for (int32 i = 0; i < userSettingsFileCount; i++) {
		const BUserSettingsFileInfo* info = fUserSettingsFileInfos.ItemAt(i);
		status_t error = info->InitCheck();
		if (error != B_OK)
			return error;
	}

	// check users
	int32 userCount = fUsers.CountItems();
	for (int32 i = 0; i < userCount; i++) {
		const BUser* user = fUsers.ItemAt(i);
		status_t error = user->InitCheck();
		if (error != B_OK)
			return B_NO_INIT;

		// make sure the user's groups are specified as groups
		const BStringList& userGroups = user->Groups();
		int32 groupCount = userGroups.CountStrings();
		for (int32 k = 0; k < groupCount; k++) {
			const BString& group = userGroups.StringAt(k);
			if (!fGroups.HasString(group))
				return B_BAD_VALUE;
		}
	}

	// check groups
	int32 groupCount = fGroups.CountStrings();
	for (int32 i = 0; i< groupCount; i++) {
		if (!BUser::IsValidUserName(fGroups.StringAt(i)))
			return B_BAD_VALUE;
	}

	return B_OK;
}


const BString&
BPackageInfo::Name() const
{
	return fName;
}


const BString&
BPackageInfo::Summary() const
{
	return fSummary;
}


const BString&
BPackageInfo::Description() const
{
	return fDescription;
}


const BString&
BPackageInfo::Vendor() const
{
	return fVendor;
}


const BString&
BPackageInfo::Packager() const
{
	return fPackager;
}


const BString&
BPackageInfo::BasePackage() const
{
	return fBasePackage;
}


const BString&
BPackageInfo::Checksum() const
{
	return fChecksum;
}


const BString&
BPackageInfo::InstallPath() const
{
	return fInstallPath;
}


BString
BPackageInfo::FileName() const
{
	return fFileName.IsEmpty() ? CanonicalFileName() : fFileName;
}


uint32
BPackageInfo::Flags() const
{
	return fFlags;
}


BPackageArchitecture
BPackageInfo::Architecture() const
{
	return fArchitecture;
}


const char*
BPackageInfo::ArchitectureName() const
{
	if ((int)fArchitecture < 0
		|| fArchitecture >= B_PACKAGE_ARCHITECTURE_ENUM_COUNT) {
		return NULL;
	}
	return kArchitectureNames[fArchitecture];
}


const BPackageVersion&
BPackageInfo::Version() const
{
	return fVersion;
}


const BStringList&
BPackageInfo::CopyrightList() const
{
	return fCopyrightList;
}


const BStringList&
BPackageInfo::LicenseList() const
{
	return fLicenseList;
}


const BStringList&
BPackageInfo::URLList() const
{
	return fURLList;
}


const BStringList&
BPackageInfo::SourceURLList() const
{
	return fSourceURLList;
}


const BObjectList<BGlobalWritableFileInfo>&
BPackageInfo::GlobalWritableFileInfos() const
{
	return fGlobalWritableFileInfos;
}


const BObjectList<BUserSettingsFileInfo>&
BPackageInfo::UserSettingsFileInfos() const
{
	return fUserSettingsFileInfos;
}


const BObjectList<BUser>&
BPackageInfo::Users() const
{
	return fUsers;
}


const BStringList&
BPackageInfo::Groups() const
{
	return fGroups;
}


const BStringList&
BPackageInfo::PostInstallScripts() const
{
	return fPostInstallScripts;
}


const BObjectList<BPackageResolvable>&
BPackageInfo::ProvidesList() const
{
	return fProvidesList;
}


const BObjectList<BPackageResolvableExpression>&
BPackageInfo::RequiresList() const
{
	return fRequiresList;
}


const BObjectList<BPackageResolvableExpression>&
BPackageInfo::SupplementsList() const
{
	return fSupplementsList;
}


const BObjectList<BPackageResolvableExpression>&
BPackageInfo::ConflictsList() const
{
	return fConflictsList;
}


const BObjectList<BPackageResolvableExpression>&
BPackageInfo::FreshensList() const
{
	return fFreshensList;
}


const BStringList&
BPackageInfo::ReplacesList() const
{
	return fReplacesList;
}


BString
BPackageInfo::CanonicalFileName() const
{
	if (InitCheck() != B_OK)
		return BString();

	return BString().SetToFormat("%s-%s-%s.hpkg", fName.String(),
		fVersion.ToString().String(), kArchitectureNames[fArchitecture]);
}


bool
BPackageInfo::Matches(const BPackageResolvableExpression& expression) const
{
	// check for an explicit match on the package
	if (expression.Name().StartsWith("pkg:")) {
		return fName == expression.Name().String() + 4
			&& expression.Matches(fVersion, fVersion);
	}

	// search for a matching provides
	int32 count = fProvidesList.CountItems();
	for (int32 i = 0; i < count; i++) {
		const BPackageResolvable* provides = fProvidesList.ItemAt(i);
		if (expression.Matches(*provides))
			return true;
	}

	return false;
}


void
BPackageInfo::SetName(const BString& name)
{
	fName = name;
	fName.ToLower();
}


void
BPackageInfo::SetSummary(const BString& summary)
{
	fSummary = summary;
}


void
BPackageInfo::SetDescription(const BString& description)
{
	fDescription = description;
}


void
BPackageInfo::SetVendor(const BString& vendor)
{
	fVendor = vendor;
}


void
BPackageInfo::SetPackager(const BString& packager)
{
	fPackager = packager;
}


void
BPackageInfo::SetBasePackage(const BString& basePackage)
{
	fBasePackage = basePackage;
}


void
BPackageInfo::SetChecksum(const BString& checksum)
{
	fChecksum = checksum;
}


void
BPackageInfo::SetInstallPath(const BString& installPath)
{
	fInstallPath = installPath;
}


void
BPackageInfo::SetFileName(const BString& fileName)
{
	fFileName = fileName;
}


void
BPackageInfo::SetVersion(const BPackageVersion& version)
{
	fVersion = version;
}


void
BPackageInfo::SetFlags(uint32 flags)
{
	fFlags = flags;
}


void
BPackageInfo::SetArchitecture(BPackageArchitecture architecture)
{
	fArchitecture = architecture;
}


void
BPackageInfo::ClearCopyrightList()
{
	fCopyrightList.MakeEmpty();
}


status_t
BPackageInfo::AddCopyright(const BString& copyright)
{
	return fCopyrightList.Add(copyright) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearLicenseList()
{
	fLicenseList.MakeEmpty();
}


status_t
BPackageInfo::AddLicense(const BString& license)
{
	return fLicenseList.Add(license) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearURLList()
{
	fURLList.MakeEmpty();
}


status_t
BPackageInfo::AddURL(const BString& url)
{
	return fURLList.Add(url) ? B_OK : B_NO_MEMORY;
}


void
BPackageInfo::ClearSourceURLList()
{
	fSourceURLList.MakeEmpty();
}


status_t
BPackageInfo::AddSourceURL(const BString& url)
{
	return fSourceURLList.Add(url) ? B_OK : B_NO_MEMORY;
}


void
BPackageInfo::ClearGlobalWritableFileInfos()
{
	fGlobalWritableFileInfos.MakeEmpty();
}


status_t
BPackageInfo::AddGlobalWritableFileInfo(const BGlobalWritableFileInfo& info)
{
	BGlobalWritableFileInfo* newInfo
		= new (std::nothrow) BGlobalWritableFileInfo(info);
	if (newInfo == NULL || !fGlobalWritableFileInfos.AddItem(newInfo)) {
		delete newInfo;
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
BPackageInfo::ClearUserSettingsFileInfos()
{
	fUserSettingsFileInfos.MakeEmpty();
}


status_t
BPackageInfo::AddUserSettingsFileInfo(const BUserSettingsFileInfo& info)
{
	BUserSettingsFileInfo* newInfo
		= new (std::nothrow) BUserSettingsFileInfo(info);
	if (newInfo == NULL || !fUserSettingsFileInfos.AddItem(newInfo)) {
		delete newInfo;
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
BPackageInfo::ClearUsers()
{
	fUsers.MakeEmpty();
}


status_t
BPackageInfo::AddUser(const BUser& user)
{
	BUser* newUser = new (std::nothrow) BUser(user);
	if (newUser == NULL || !fUsers.AddItem(newUser)) {
		delete newUser;
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
BPackageInfo::ClearGroups()
{
	fGroups.MakeEmpty();
}


status_t
BPackageInfo::AddGroup(const BString& group)
{
	return fGroups.Add(group) ? B_OK : B_NO_MEMORY;
}


void
BPackageInfo::ClearPostInstallScripts()
{
	fPostInstallScripts.MakeEmpty();
}


status_t
BPackageInfo::AddPostInstallScript(const BString& path)
{
	return fPostInstallScripts.Add(path) ? B_OK : B_NO_MEMORY;
}


void
BPackageInfo::ClearProvidesList()
{
	fProvidesList.MakeEmpty();
}


status_t
BPackageInfo::AddProvides(const BPackageResolvable& provides)
{
	BPackageResolvable* newProvides
		= new (std::nothrow) BPackageResolvable(provides);
	if (newProvides == NULL)
		return B_NO_MEMORY;

	return fProvidesList.AddItem(newProvides) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearRequiresList()
{
	fRequiresList.MakeEmpty();
}


status_t
BPackageInfo::AddRequires(const BPackageResolvableExpression& requires)
{
	BPackageResolvableExpression* newRequires
		= new (std::nothrow) BPackageResolvableExpression(requires);
	if (newRequires == NULL)
		return B_NO_MEMORY;

	return fRequiresList.AddItem(newRequires) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearSupplementsList()
{
	fSupplementsList.MakeEmpty();
}


status_t
BPackageInfo::AddSupplements(const BPackageResolvableExpression& supplements)
{
	BPackageResolvableExpression* newSupplements
		= new (std::nothrow) BPackageResolvableExpression(supplements);
	if (newSupplements == NULL)
		return B_NO_MEMORY;

	return fSupplementsList.AddItem(newSupplements) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearConflictsList()
{
	fConflictsList.MakeEmpty();
}


status_t
BPackageInfo::AddConflicts(const BPackageResolvableExpression& conflicts)
{
	BPackageResolvableExpression* newConflicts
		= new (std::nothrow) BPackageResolvableExpression(conflicts);
	if (newConflicts == NULL)
		return B_NO_MEMORY;

	return fConflictsList.AddItem(newConflicts) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearFreshensList()
{
	fFreshensList.MakeEmpty();
}


status_t
BPackageInfo::AddFreshens(const BPackageResolvableExpression& freshens)
{
	BPackageResolvableExpression* newFreshens
		= new (std::nothrow) BPackageResolvableExpression(freshens);
	if (newFreshens == NULL)
		return B_NO_MEMORY;

	return fFreshensList.AddItem(newFreshens) ? B_OK : B_ERROR;
}


void
BPackageInfo::ClearReplacesList()
{
	fReplacesList.MakeEmpty();
}


status_t
BPackageInfo::AddReplaces(const BString& replaces)
{
	return fReplacesList.Add(BString(replaces).ToLower()) ? B_OK : B_ERROR;
}


void
BPackageInfo::Clear()
{
	fName.Truncate(0);
	fSummary.Truncate(0);
	fDescription.Truncate(0);
	fVendor.Truncate(0);
	fPackager.Truncate(0);
	fBasePackage.Truncate(0);
	fChecksum.Truncate(0);
	fInstallPath.Truncate(0);
	fFileName.Truncate(0);
	fFlags = 0;
	fArchitecture = B_PACKAGE_ARCHITECTURE_ENUM_COUNT;
	fVersion.Clear();
	fCopyrightList.MakeEmpty();
	fLicenseList.MakeEmpty();
	fURLList.MakeEmpty();
	fSourceURLList.MakeEmpty();
	fGlobalWritableFileInfos.MakeEmpty();
	fUserSettingsFileInfos.MakeEmpty();
	fUsers.MakeEmpty();
	fGroups.MakeEmpty();
	fPostInstallScripts.MakeEmpty();
	fRequiresList.MakeEmpty();
	fProvidesList.MakeEmpty();
	fSupplementsList.MakeEmpty();
	fConflictsList.MakeEmpty();
	fFreshensList.MakeEmpty();
	fReplacesList.MakeEmpty();
}


status_t
BPackageInfo::Archive(BMessage* archive, bool deep) const
{
	status_t error = BArchivable::Archive(archive, deep);
	if (error != B_OK)
		return error;

	if ((error = archive->AddString("name", fName)) != B_OK
		|| (error = archive->AddString("summary", fSummary)) != B_OK
		|| (error = archive->AddString("description", fDescription)) != B_OK
		|| (error = archive->AddString("vendor", fVendor)) != B_OK
		|| (error = archive->AddString("packager", fPackager)) != B_OK
		|| (error = archive->AddString("basePackage", fBasePackage)) != B_OK
		|| (error = archive->AddUInt32("flags", fFlags)) != B_OK
		|| (error = archive->AddInt32("architecture", fArchitecture)) != B_OK
		|| (error = _AddVersion(archive, "version", fVersion)) != B_OK
		|| (error = archive->AddStrings("copyrights", fCopyrightList))
			!= B_OK
		|| (error = archive->AddStrings("licenses", fLicenseList)) != B_OK
		|| (error = archive->AddStrings("urls", fURLList)) != B_OK
		|| (error = archive->AddStrings("source-urls", fSourceURLList))
			!= B_OK
		|| (error = _AddGlobalWritableFileInfos(archive,
			"global-writable-files", fGlobalWritableFileInfos)) != B_OK
		|| (error = _AddUserSettingsFileInfos(archive,
			"user-settings-files", fUserSettingsFileInfos)) != B_OK
		|| (error = _AddUsers(archive, "users", fUsers)) != B_OK
		|| (error = archive->AddStrings("groups", fGroups)) != B_OK
		|| (error = archive->AddStrings("post-install-scripts",
			fPostInstallScripts)) != B_OK
		|| (error = _AddResolvables(archive, "provides", fProvidesList)) != B_OK
		|| (error = _AddResolvableExpressions(archive, "requires",
			fRequiresList)) != B_OK
		|| (error = _AddResolvableExpressions(archive, "supplements",
			fSupplementsList)) != B_OK
		|| (error = _AddResolvableExpressions(archive, "conflicts",
			fConflictsList)) != B_OK
		|| (error = _AddResolvableExpressions(archive, "freshens",
			fFreshensList)) != B_OK
		|| (error = archive->AddStrings("replaces", fReplacesList)) != B_OK
		|| (error = archive->AddString("checksum", fChecksum)) != B_OK
		|| (error = archive->AddString("install-path", fInstallPath)) != B_OK
		|| (error = archive->AddString("file-name", fFileName)) != B_OK) {
		return error;
	}

	return B_OK;
}


/*static*/ BArchivable*
BPackageInfo::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BPackageInfo"))
		return new(std::nothrow) BPackageInfo(archive);
	return NULL;
}


status_t
BPackageInfo::GetConfigString(BString& _string) const
{
	return StringBuilder()
		.Write("name", fName)
		.Write("version", fVersion)
		.Write("summary", fSummary)
		.Write("description", fDescription)
		.Write("vendor", fVendor)
		.Write("packager", fPackager)
		.Write("architecture", kArchitectureNames[fArchitecture])
		.Write("copyrights", fCopyrightList)
		.Write("licenses", fLicenseList)
		.Write("urls", fURLList)
		.Write("source-urls", fSourceURLList)
		.Write("global-writable-files", fGlobalWritableFileInfos)
		.Write("user-settings-files", fUserSettingsFileInfos)
		.Write("users", fUsers)
		.Write("groups", fGroups)
		.Write("post-install-scripts", fPostInstallScripts)
		.Write("provides", fProvidesList)
		.BeginRequires(fBasePackage)
			.Write("requires", fRequiresList)
		.EndRequires()
		.Write("supplements", fSupplementsList)
		.Write("conflicts", fConflictsList)
		.Write("freshens", fFreshensList)
		.Write("replaces", fReplacesList)
		.WriteFlags("flags", fFlags)
		.Write("checksum", fChecksum)
		.GetString(_string);
	// Note: fInstallPath and fFileName can not be specified via .PackageInfo.
}


BString
BPackageInfo::ToString() const
{
	BString string;
	GetConfigString(string);
	return string;
}


/*static*/ status_t
BPackageInfo::GetArchitectureByName(const BString& name,
	BPackageArchitecture& _architecture)
{
	for (int i = 0; i < B_PACKAGE_ARCHITECTURE_ENUM_COUNT; ++i) {
		if (name.ICompare(kArchitectureNames[i]) == 0) {
			_architecture = (BPackageArchitecture)i;
			return B_OK;
		}
	}
	return B_NAME_NOT_FOUND;
}


/*static*/ status_t
BPackageInfo::ParseVersionString(const BString& string, bool revisionIsOptional,
	BPackageVersion& _version, ParseErrorListener* listener)
{
	return Parser(listener).ParseVersion(string, revisionIsOptional, _version);
}


/*static*/ status_t
BPackageInfo::ParseResolvableExpressionString(const BString& string,
	BPackageResolvableExpression& _expression, ParseErrorListener* listener)
{
	return Parser(listener).ParseResolvableExpression(string, _expression);
}


status_t
BPackageInfo::_ReadFromPackageFile(const PackageFileLocation& fileLocation)
{
	BHPKG::BNoErrorOutput errorOutput;

	// try current package file format version
	{
		BHPKG::BPackageReader packageReader(&errorOutput);
		status_t error = fileLocation.Path() != NULL
			? packageReader.Init(fileLocation.Path())
			: packageReader.Init(fileLocation.FD(), false);
		if (error == B_OK) {
			BPackageInfoContentHandler handler(*this);
			return packageReader.ParseContent(&handler);
		}

		if (error != B_MISMATCHED_VALUES)
			return error;
	}

	// try package file format version 1
	BHPKG::V1::BPackageReader packageReader(&errorOutput);
	status_t error = fileLocation.Path() != NULL
		? packageReader.Init(fileLocation.Path())
		: packageReader.Init(fileLocation.FD(), false);
	if (error != B_OK)
		return error;

	BHPKG::V1::BPackageInfoContentHandler handler(*this);
	return packageReader.ParseContent(&handler);
}


/*static*/ status_t
BPackageInfo::_AddVersion(BMessage* archive, const char* field,
	const BPackageVersion& version)
{
	// Storing BPackageVersion::ToString() would be nice, but the corresponding
	// constructor only works for valid versions and we might want to store
	// invalid versions as well.

	// major
	size_t fieldLength = strlen(field);
	FieldName fieldName(field, ":major");
	if (!fieldName.IsValid())
		return B_BAD_VALUE;

	status_t error = archive->AddString(fieldName, version.Major());
	if (error != B_OK)
		return error;

	// minor
	if (!fieldName.ReplaceSuffix(fieldLength, ":minor"))
		return B_BAD_VALUE;

	error = archive->AddString(fieldName, version.Minor());
	if (error != B_OK)
		return error;

	// micro
	if (!fieldName.ReplaceSuffix(fieldLength, ":micro"))
		return B_BAD_VALUE;

	error = archive->AddString(fieldName, version.Micro());
	if (error != B_OK)
		return error;

	// pre-release
	if (!fieldName.ReplaceSuffix(fieldLength, ":pre"))
		return B_BAD_VALUE;

	error = archive->AddString(fieldName, version.PreRelease());
	if (error != B_OK)
		return error;

	// revision
	if (!fieldName.ReplaceSuffix(fieldLength, ":revision"))
		return B_BAD_VALUE;

	return archive->AddUInt32(fieldName, version.Revision());
}


/*static*/ status_t
BPackageInfo::_AddResolvables(BMessage* archive, const char* field,
	const ResolvableList& resolvables)
{
	// construct the field names we need
	FieldName nameField(field, ":name");
	FieldName typeField(field, ":type");
	FieldName versionField(field, ":version");
	FieldName compatibleVersionField(field, ":compat");

	if (!nameField.IsValid() || !typeField.IsValid() || !versionField.IsValid()
		|| !compatibleVersionField.IsValid()) {
		return B_BAD_VALUE;
	}

	// add fields
	int32 count = resolvables.CountItems();
	for (int32 i = 0; i < count; i++) {
		const BPackageResolvable* resolvable = resolvables.ItemAt(i);
		status_t error;
		if ((error = archive->AddString(nameField, resolvable->Name())) != B_OK
			|| (error = _AddVersion(archive, versionField,
				resolvable->Version())) != B_OK
			|| (error = _AddVersion(archive, compatibleVersionField,
				resolvable->CompatibleVersion())) != B_OK) {
			return error;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPackageInfo::_AddResolvableExpressions(BMessage* archive, const char* field,
	const ResolvableExpressionList& expressions)
{
	// construct the field names we need
	FieldName nameField(field, ":name");
	FieldName operatorField(field, ":operator");
	FieldName versionField(field, ":version");

	if (!nameField.IsValid() || !operatorField.IsValid()
		|| !versionField.IsValid()) {
		return B_BAD_VALUE;
	}

	// add fields
	int32 count = expressions.CountItems();
	for (int32 i = 0; i < count; i++) {
		const BPackageResolvableExpression* expression = expressions.ItemAt(i);
		status_t error;
		if ((error = archive->AddString(nameField, expression->Name())) != B_OK
			|| (error = archive->AddInt32(operatorField,
				expression->Operator())) != B_OK
			|| (error = _AddVersion(archive, versionField,
				expression->Version())) != B_OK) {
			return error;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPackageInfo::_AddGlobalWritableFileInfos(BMessage* archive, const char* field,
	const GlobalWritableFileInfoList& infos)
{
	// construct the field names we need
	FieldName pathField(field, ":path");
	FieldName updateTypeField(field, ":updateType");
	FieldName isDirectoryField(field, ":isDirectory");

	if (!pathField.IsValid() || !updateTypeField.IsValid()
		|| !isDirectoryField.IsValid()) {
		return B_BAD_VALUE;
	}

	// add fields
	int32 count = infos.CountItems();
	for (int32 i = 0; i < count; i++) {
		const BGlobalWritableFileInfo* info = infos.ItemAt(i);
		status_t error;
		if ((error = archive->AddString(pathField, info->Path())) != B_OK
			|| (error = archive->AddInt32(updateTypeField, info->UpdateType()))
				!= B_OK
			|| (error = archive->AddBool(isDirectoryField,
				info->IsDirectory())) != B_OK) {
			return error;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPackageInfo::_AddUserSettingsFileInfos(BMessage* archive, const char* field,
	const UserSettingsFileInfoList& infos)
{
	// construct the field names we need
	FieldName pathField(field, ":path");
	FieldName templatePathField(field, ":templatePath");
	FieldName isDirectoryField(field, ":isDirectory");

	if (!pathField.IsValid() || !templatePathField.IsValid()
		|| !isDirectoryField.IsValid()) {
		return B_BAD_VALUE;
	}

	// add fields
	int32 count = infos.CountItems();
	for (int32 i = 0; i < count; i++) {
		const BUserSettingsFileInfo* info = infos.ItemAt(i);
		status_t error;
		if ((error = archive->AddString(pathField, info->Path())) != B_OK
			|| (error = archive->AddString(templatePathField,
				info->TemplatePath())) != B_OK
			|| (error = archive->AddBool(isDirectoryField,
				info->IsDirectory())) != B_OK) {
			return error;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPackageInfo::_AddUsers(BMessage* archive, const char* field,
	const UserList& users)
{
	// construct the field names we need
	FieldName nameField(field, ":name");
	FieldName realNameField(field, ":realName");
	FieldName homeField(field, ":home");
	FieldName shellField(field, ":shell");
	FieldName groupsField(field, ":groups");

	if (!nameField.IsValid() || !realNameField.IsValid() || !homeField.IsValid()
		|| !shellField.IsValid() || !groupsField.IsValid())
		return B_BAD_VALUE;

	// add fields
	int32 count = users.CountItems();
	for (int32 i = 0; i < count; i++) {
		const BUser* user = users.ItemAt(i);
		BString groups = user->Groups().Join(" ");
		if (groups.IsEmpty() && !user->Groups().IsEmpty())
			return B_NO_MEMORY;

		status_t error;
		if ((error = archive->AddString(nameField, user->Name())) != B_OK
			|| (error = archive->AddString(realNameField, user->RealName()))
				!= B_OK
			|| (error = archive->AddString(homeField, user->Home())) != B_OK
			|| (error = archive->AddString(shellField, user->Shell())) != B_OK
			|| (error = archive->AddString(groupsField, groups)) != B_OK) {
			return error;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPackageInfo::_ExtractVersion(BMessage* archive, const char* field, int32 index,
	BPackageVersion& _version)
{
	// major
	size_t fieldLength = strlen(field);
	FieldName fieldName(field, ":major");
	if (!fieldName.IsValid())
		return B_BAD_VALUE;

	BString major;
	status_t error = archive->FindString(fieldName, index, &major);
	if (error != B_OK)
		return error;

	// minor
	if (!fieldName.ReplaceSuffix(fieldLength, ":minor"))
		return B_BAD_VALUE;

	BString minor;
	error = archive->FindString(fieldName, index, &minor);
	if (error != B_OK)
		return error;

	// micro
	if (!fieldName.ReplaceSuffix(fieldLength, ":micro"))
		return B_BAD_VALUE;

	BString micro;
	error = archive->FindString(fieldName, index, &micro);
	if (error != B_OK)
		return error;

	// pre-release
	if (!fieldName.ReplaceSuffix(fieldLength, ":pre"))
		return B_BAD_VALUE;

	BString preRelease;
	error = archive->FindString(fieldName, index, &preRelease);
	if (error != B_OK)
		return error;

	// revision
	if (!fieldName.ReplaceSuffix(fieldLength, ":revision"))
		return B_BAD_VALUE;

	uint32 revision;
	error = archive->FindUInt32(fieldName, index, &revision);
	if (error != B_OK)
		return error;

	_version.SetTo(major, minor, micro, preRelease, revision);
	return B_OK;
}


/*static*/ status_t
BPackageInfo::_ExtractStringList(BMessage* archive, const char* field,
	BStringList& _list)
{
	status_t error = archive->FindStrings(field, &_list);
	return error == B_NAME_NOT_FOUND ? B_OK : error;
		// If the field doesn't exist, that's OK.
}


/*static*/ status_t
BPackageInfo::_ExtractResolvables(BMessage* archive, const char* field,
	ResolvableList& _resolvables)
{
	// construct the field names we need
	FieldName nameField(field, ":name");
	FieldName typeField(field, ":type");
	FieldName versionField(field, ":version");
	FieldName compatibleVersionField(field, ":compat");

	if (!nameField.IsValid() || !typeField.IsValid() || !versionField.IsValid()
		|| !compatibleVersionField.IsValid()) {
		return B_BAD_VALUE;
	}

	// get the number of items
	type_code type;
	int32 count;
	if (archive->GetInfo(nameField, &type, &count) != B_OK) {
		// the field is missing
		return B_OK;
	}

	// extract fields
	for (int32 i = 0; i < count; i++) {
		BString name;
		status_t error = archive->FindString(nameField, i, &name);
		if (error != B_OK)
			return error;

		BPackageVersion version;
		error = _ExtractVersion(archive, versionField, i, version);
		if (error != B_OK)
			return error;

		BPackageVersion compatibleVersion;
		error = _ExtractVersion(archive, compatibleVersionField, i,
			compatibleVersion);
		if (error != B_OK)
			return error;

		BPackageResolvable* resolvable = new(std::nothrow) BPackageResolvable(
			name, version, compatibleVersion);
		if (resolvable == NULL || !_resolvables.AddItem(resolvable)) {
			delete resolvable;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPackageInfo::_ExtractResolvableExpressions(BMessage* archive,
	const char* field, ResolvableExpressionList& _expressions)
{
	// construct the field names we need
	FieldName nameField(field, ":name");
	FieldName operatorField(field, ":operator");
	FieldName versionField(field, ":version");

	if (!nameField.IsValid() || !operatorField.IsValid()
		|| !versionField.IsValid()) {
		return B_BAD_VALUE;
	}

	// get the number of items
	type_code type;
	int32 count;
	if (archive->GetInfo(nameField, &type, &count) != B_OK) {
		// the field is missing
		return B_OK;
	}

	// extract fields
	for (int32 i = 0; i < count; i++) {
		BString name;
		status_t error = archive->FindString(nameField, i, &name);
		if (error != B_OK)
			return error;

		int32 operatorType;
		error = archive->FindInt32(operatorField, i, &operatorType);
		if (error != B_OK)
			return error;
		if (operatorType < 0
			|| operatorType > B_PACKAGE_RESOLVABLE_OP_ENUM_COUNT) {
			return B_BAD_DATA;
		}

		BPackageVersion version;
		error = _ExtractVersion(archive, versionField, i, version);
		if (error != B_OK)
			return error;

		BPackageResolvableExpression* expression
			= new(std::nothrow) BPackageResolvableExpression(name,
				(BPackageResolvableOperator)operatorType, version);
		if (expression == NULL || !_expressions.AddItem(expression)) {
			delete expression;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPackageInfo::_ExtractGlobalWritableFileInfos(BMessage* archive,
	const char* field, GlobalWritableFileInfoList& _infos)
{
	// construct the field names we need
	FieldName pathField(field, ":path");
	FieldName updateTypeField(field, ":updateType");
	FieldName isDirectoryField(field, ":isDirectory");

	if (!pathField.IsValid() || !updateTypeField.IsValid()
		|| !isDirectoryField.IsValid()) {
		return B_BAD_VALUE;
	}

	// get the number of items
	type_code type;
	int32 count;
	if (archive->GetInfo(pathField, &type, &count) != B_OK) {
		// the field is missing
		return B_OK;
	}

	// extract fields
	for (int32 i = 0; i < count; i++) {
		BString path;
		status_t error = archive->FindString(pathField, i, &path);
		if (error != B_OK)
			return error;

		int32 updateType;
		error = archive->FindInt32(updateTypeField, i, &updateType);
		if (error != B_OK)
			return error;
		if (updateType < 0
			|| updateType > B_WRITABLE_FILE_UPDATE_TYPE_ENUM_COUNT) {
			return B_BAD_DATA;
		}

		bool isDirectory;
		error = archive->FindBool(isDirectoryField, i, &isDirectory);
		if (error != B_OK)
			return error;

		BGlobalWritableFileInfo* info
			= new(std::nothrow) BGlobalWritableFileInfo(path,
				(BWritableFileUpdateType)updateType, isDirectory);
		if (info == NULL || !_infos.AddItem(info)) {
			delete info;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPackageInfo::_ExtractUserSettingsFileInfos(BMessage* archive,
	const char* field, UserSettingsFileInfoList& _infos)
{
	// construct the field names we need
	FieldName pathField(field, ":path");
	FieldName templatePathField(field, ":templatePath");
	FieldName isDirectoryField(field, ":isDirectory");

	if (!pathField.IsValid() || !templatePathField.IsValid()
		|| !isDirectoryField.IsValid()) {
		return B_BAD_VALUE;
	}

	// get the number of items
	type_code type;
	int32 count;
	if (archive->GetInfo(pathField, &type, &count) != B_OK) {
		// the field is missing
		return B_OK;
	}

	// extract fields
	for (int32 i = 0; i < count; i++) {
		BString path;
		status_t error = archive->FindString(pathField, i, &path);
		if (error != B_OK)
			return error;

		BString templatePath;
		error = archive->FindString(templatePathField, i, &templatePath);
		if (error != B_OK)
			return error;

		bool isDirectory;
		error = archive->FindBool(isDirectoryField, i, &isDirectory);
		if (error != B_OK)
			return error;

		BUserSettingsFileInfo* info = isDirectory
			? new(std::nothrow) BUserSettingsFileInfo(path, true)
			: new(std::nothrow) BUserSettingsFileInfo(path, templatePath);
		if (info == NULL || !_infos.AddItem(info)) {
			delete info;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPackageInfo::_ExtractUsers(BMessage* archive, const char* field,
	UserList& _users)
{
	// construct the field names we need
	FieldName nameField(field, ":name");
	FieldName realNameField(field, ":realName");
	FieldName homeField(field, ":home");
	FieldName shellField(field, ":shell");
	FieldName groupsField(field, ":groups");

	if (!nameField.IsValid() || !realNameField.IsValid() || !homeField.IsValid()
		|| !shellField.IsValid() || !groupsField.IsValid())
		return B_BAD_VALUE;

	// get the number of items
	type_code type;
	int32 count;
	if (archive->GetInfo(nameField, &type, &count) != B_OK) {
		// the field is missing
		return B_OK;
	}

	// extract fields
	for (int32 i = 0; i < count; i++) {
		BString name;
		status_t error = archive->FindString(nameField, i, &name);
		if (error != B_OK)
			return error;

		BString realName;
		error = archive->FindString(realNameField, i, &realName);
		if (error != B_OK)
			return error;

		BString home;
		error = archive->FindString(homeField, i, &home);
		if (error != B_OK)
			return error;

		BString shell;
		error = archive->FindString(shellField, i, &shell);
		if (error != B_OK)
			return error;

		BString groupsString;
		error = archive->FindString(groupsField, i, &groupsString);
		if (error != B_OK)
			return error;

		BStringList groups;
		if (!groupsString.IsEmpty() && !groupsString.Split(" ", false, groups))
			return B_NO_MEMORY;

		BUser* user = new(std::nothrow) BUser(name, realName, home, shell,
			groups);
		if (user == NULL || !_users.AddItem(user)) {
			delete user;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


}	// namespace BPackageKit
