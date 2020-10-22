/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2014, Axel Dörfler <axeld@pinc-software.de>.
 * Copyright 2016-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "Model.h"

#include <algorithm>
#include <ctime>
#include <vector>

#include <stdarg.h>
#include <time.h>

#include <Autolock.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <KeyStore.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <Message.h>
#include <Path.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "LocaleUtils.h"
#include "StorageUtils.h"
#include "RepositoryUrlUtils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Model"


#define KEY_STORE_IDENTIFIER_PREFIX "hds.password."
	// this prefix is added before the nickname in the keystore
	// so that HDS username/password pairs can be identified.

static const char* kHaikuDepotKeyring = "HaikuDepot";


PackageFilter::~PackageFilter()
{
}


ModelListener::~ModelListener()
{
}


// #pragma mark - PackageFilters


class AnyFilter : public PackageFilter {
public:
	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		return true;
	}
};


class DepotFilter : public PackageFilter {
public:
	DepotFilter(const DepotInfo& depot)
		:
		fDepot(depot)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		// TODO: Maybe a PackageInfo ought to know the Depot it came from?
		// But right now the same package could theoretically be provided
		// from different depots and the filter would work correctly.
		// Also the PackageList could actually contain references to packages
		// instead of the packages as objects. The equal operator is quite
		// expensive as is.
		return fDepot.Packages().Contains(package);
	}

	const BString& Depot() const
	{
		return fDepot.Name();
	}

private:
	DepotInfo	fDepot;
};


class CategoryFilter : public PackageFilter {
public:
	CategoryFilter(const BString& category)
		:
		fCategory(category)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		if (package.Get() == NULL)
			return false;

		for (int i = package->CountCategories() - 1; i >= 0; i--) {
			const CategoryRef& category = package->CategoryAtIndex(i);
			if (category.Get() == NULL)
				continue;
			if (category->Code() == fCategory)
				return true;
		}
		return false;
	}

	const BString& Category() const
	{
		return fCategory;
	}

private:
	BString		fCategory;
};


class ContainedInFilter : public PackageFilter {
public:
	ContainedInFilter(const PackageList& packageList)
		:
		fPackageList(packageList)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		return fPackageList.Contains(package);
	}

private:
	const PackageList&	fPackageList;
};


class ContainedInEitherFilter : public PackageFilter {
public:
	ContainedInEitherFilter(const PackageList& packageListA,
		const PackageList& packageListB)
		:
		fPackageListA(packageListA),
		fPackageListB(packageListB)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		return fPackageListA.Contains(package)
			|| fPackageListB.Contains(package);
	}

private:
	const PackageList&	fPackageListA;
	const PackageList&	fPackageListB;
};


class NotContainedInFilter : public PackageFilter {
public:
	NotContainedInFilter(const PackageList* packageList, ...)
	{
		va_list args;
		va_start(args, packageList);
		while (true) {
			const PackageList* packageList = va_arg(args, const PackageList*);
			if (packageList == NULL)
				break;
			fPackageLists.Add(packageList);
		}
		va_end(args);
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		if (package.Get() == NULL)
			return false;

		for (int32 i = 0; i < fPackageLists.CountItems(); i++) {
			if (fPackageLists.ItemAtFast(i)->Contains(package))
				return false;
		}
		return true;
	}

private:
	List<const PackageList*, true>	fPackageLists;
};


class StateFilter : public PackageFilter {
public:
	StateFilter(PackageState state)
		:
		fState(state)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		return package->State() == NONE;
	}

private:
	PackageState	fState;
};


class SearchTermsFilter : public PackageFilter {
public:
	SearchTermsFilter(const BString& searchTerms)
	{
		// Separate the string into terms at spaces
		int32 index = 0;
		while (index < searchTerms.Length()) {
			int32 nextSpace = searchTerms.FindFirst(" ", index);
			if (nextSpace < 0)
				nextSpace = searchTerms.Length();
			if (nextSpace > index) {
				BString term;
				searchTerms.CopyInto(term, index, nextSpace - index);
				term.ToLower();
				fSearchTerms.Add(term);
			}
			index = nextSpace + 1;
		}
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		if (package.Get() == NULL)
			return false;
		// Every search term must be found in one of the package texts
		for (int32 i = fSearchTerms.CountStrings() - 1; i >= 0; i--) {
			const BString& term = fSearchTerms.StringAt(i);
			if (!_TextContains(package->Name(), term)
				&& !_TextContains(package->Title(), term)
				&& !_TextContains(package->Publisher().Name(), term)
				&& !_TextContains(package->ShortDescription(), term)
				&& !_TextContains(package->FullDescription(), term)) {
				return false;
			}
		}
		return true;
	}

	BString SearchTerms() const
	{
		BString searchTerms;
		for (int32 i = 0; i < fSearchTerms.CountStrings(); i++) {
			const BString& term = fSearchTerms.StringAt(i);
			if (term.IsEmpty())
				continue;
			if (!searchTerms.IsEmpty())
				searchTerms.Append(" ");
			searchTerms.Append(term);
		}
		return searchTerms;
	}

private:
	bool _TextContains(BString text, const BString& string) const
	{
		text.ToLower();
		int32 index = text.FindFirst(string);
		return index >= 0;
	}

private:
	BStringList fSearchTerms;
};


static inline bool
is_source_package(const PackageInfoRef& package)
{
	const BString& packageName = package->Name();
	return packageName.EndsWith("_source");
}


static inline bool
is_develop_package(const PackageInfoRef& package)
{
	const BString& packageName = package->Name();
	return packageName.EndsWith("_devel")
		|| packageName.EndsWith("_debuginfo");
}


// #pragma mark - Model


Model::Model()
	:
	fDepots(),
	fCategories(),
	fCategoryFilter(PackageFilterRef(new AnyFilter(), true)),
	fDepotFilter(""),
	fSearchTermsFilter(PackageFilterRef(new AnyFilter(), true)),
	fPackageListViewMode(PROMINENT),
	fShowAvailablePackages(true),
	fShowInstalledPackages(true),
	fShowSourcePackages(false),
	fShowDevelopPackages(false)
{
}


Model::~Model()
{
}


LanguageModel*
Model::Language()
{
	return &fLanguageModel;
}


PackageIconRepository&
Model::GetPackageIconRepository()
{
	return fPackageIconRepository;
}


status_t
Model::InitPackageIconRepository()
{
	BPath tarPath;
	status_t result = IconTarPath(tarPath);
	if (result == B_OK)
		result = fPackageIconRepository.Init(tarPath);
	return result;
}


bool
Model::AddListener(const ModelListenerRef& listener)
{
	return fListeners.Add(listener);
}


// TODO; part of a wider change; cope with the package being in more than one
// depot
PackageInfoRef
Model::PackageForName(const BString& name)
{
	std::vector<DepotInfoRef>::iterator it;
	for (it = fDepots.begin(); it != fDepots.end(); it++) {
		DepotInfoRef depotInfoRef = *it;
		int32 packageIndex = depotInfoRef->PackageIndexByName(name);
		if (packageIndex >= 0)
			return depotInfoRef->Packages().ItemAtFast(packageIndex);
	}
	return PackageInfoRef();
}


bool
Model::MatchesFilter(const PackageInfoRef& package) const
{
	return fCategoryFilter->AcceptsPackage(package)
			&& fSearchTermsFilter->AcceptsPackage(package)
			&& (fDepotFilter.IsEmpty() || fDepotFilter == package->DepotName())
			&& (fShowAvailablePackages || package->State() != NONE)
			&& (fShowInstalledPackages || package->State() != ACTIVATED)
			&& (fShowSourcePackages || !is_source_package(package))
			&& (fShowDevelopPackages || !is_develop_package(package));
}


void
Model::MergeOrAddDepot(const DepotInfoRef depot)
{
	BString depotName = depot->Name();
	for(uint32 i = 0; i < fDepots.size(); i++) {
		if (fDepots[i]->Name() == depotName) {
			DepotInfoRef ersatzDepot(new DepotInfo(*(fDepots[i].Get())), true);
			ersatzDepot->SyncPackages(depot->Packages());
			fDepots[i] = ersatzDepot;
			return;
		}
	}
	fDepots.push_back(depot);
}


bool
Model::HasDepot(const BString& name) const
{
	return NULL != DepotForName(name).Get();
}


const DepotInfoRef
Model::DepotForName(const BString& name) const
{
	std::vector<DepotInfoRef>::const_iterator it;
	for (it = fDepots.begin(); it != fDepots.end(); it++) {
		DepotInfoRef aDepot = *it;
		if (aDepot->Name() == name)
			return aDepot;
	}
	return DepotInfoRef();
}


int32
Model::CountDepots() const
{
	return fDepots.size();
}


DepotInfoRef
Model::DepotAtIndex(int32 index) const
{
	return fDepots[index];
}


bool
Model::HasAnyProminentPackages()
{
	std::vector<DepotInfoRef>::iterator it;
	for (it = fDepots.begin(); it != fDepots.end(); it++) {
		DepotInfoRef aDepot = *it;
		if (aDepot->HasAnyProminentPackages())
			return true;
	}
	return false;
}


void
Model::Clear()
{
	fDepots.clear();
}


void
Model::SetPackageState(const PackageInfoRef& package, PackageState state)
{
	switch (state) {
		default:
		case NONE:
			fInstalledPackages.Remove(package);
			fActivatedPackages.Remove(package);
			fUninstalledPackages.Remove(package);
			break;
		case INSTALLED:
			if (!fInstalledPackages.Contains(package))
				fInstalledPackages.Add(package);
			fActivatedPackages.Remove(package);
			fUninstalledPackages.Remove(package);
			break;
		case ACTIVATED:
			if (!fInstalledPackages.Contains(package))
				fInstalledPackages.Add(package);
			if (!fActivatedPackages.Contains(package))
				fActivatedPackages.Add(package);
			fUninstalledPackages.Remove(package);
			break;
		case UNINSTALLED:
			fInstalledPackages.Remove(package);
			fActivatedPackages.Remove(package);
			if (!fUninstalledPackages.Contains(package))
				fUninstalledPackages.Add(package);
			break;
	}

	package->SetState(state);
}


// #pragma mark - filters


void
Model::SetCategory(const BString& category)
{
	PackageFilter* filter;

	if (category.Length() == 0)
		filter = new AnyFilter();
	else
		filter = new CategoryFilter(category);

	fCategoryFilter.SetTo(filter, true);
}


BString
Model::Category() const
{
	CategoryFilter* filter
		= dynamic_cast<CategoryFilter*>(fCategoryFilter.Get());
	if (filter == NULL)
		return "";
	return filter->Category();
}


void
Model::SetDepot(const BString& depot)
{
	fDepotFilter = depot;
}


BString
Model::Depot() const
{
	return fDepotFilter;
}


void
Model::SetSearchTerms(const BString& searchTerms)
{
	PackageFilter* filter;

	if (searchTerms.Length() == 0)
		filter = new AnyFilter();
	else
		filter = new SearchTermsFilter(searchTerms);

	fSearchTermsFilter.SetTo(filter, true);
}


BString
Model::SearchTerms() const
{
	SearchTermsFilter* filter
		= dynamic_cast<SearchTermsFilter*>(fSearchTermsFilter.Get());
	if (filter == NULL)
		return "";
	return filter->SearchTerms();
}


void
Model::SetPackageListViewMode(package_list_view_mode mode)
{
	fPackageListViewMode = mode;
}


void
Model::SetShowAvailablePackages(bool show)
{
	fShowAvailablePackages = show;
}


void
Model::SetShowInstalledPackages(bool show)
{
	fShowInstalledPackages = show;
}


void
Model::SetShowSourcePackages(bool show)
{
	fShowSourcePackages = show;
}


void
Model::SetShowDevelopPackages(bool show)
{
	fShowDevelopPackages = show;
}


// #pragma mark - information retrieval


/*! Initially only superficial data is loaded from the server into the data
    model of the packages.  When the package is viewed, additional data needs
    to be populated including ratings.  This method takes care of that.
*/

void
Model::PopulatePackage(const PackageInfoRef& package, uint32 flags)
{
	// TODO: There should probably also be a way to "unpopulate" the
	// package information. Maybe a cache of populated packages, so that
	// packages loose their extra information after a certain amount of
	// time when they have not been accessed/displayed in the UI. Otherwise
	// HaikuDepot will consume more and more resources in the packages.
	// Especially screen-shots will be a problem eventually.
	{
		BAutolock locker(&fLock);
		bool alreadyPopulated = fPopulatedPackages.Contains(package);
		if ((flags & POPULATE_FORCE) == 0 && alreadyPopulated)
			return;
		if (!alreadyPopulated)
			fPopulatedPackages.Add(package);
	}

	if ((flags & POPULATE_CHANGELOG) != 0 && package->HasChangelog()) {
		_PopulatePackageChangelog(package);
	}

	if ((flags & POPULATE_USER_RATINGS) != 0) {
		// Retrieve info from web-app
		BMessage info;

		BString packageName;
		BString webAppRepositoryCode;
		{
			BAutolock locker(&fLock);
			packageName = package->Name();
			const DepotInfo* depot = DepotForName(package->DepotName());

			if (depot != NULL)
				webAppRepositoryCode = depot->WebAppRepositoryCode();
		}

		status_t status = fWebAppInterface
			.RetreiveUserRatingsForPackageForDisplay(packageName,
				webAppRepositoryCode, 0, PACKAGE_INFO_MAX_USER_RATINGS,
				info);
		if (status == B_OK) {
			// Parse message
			BMessage result;
			BMessage items;
			if (info.FindMessage("result", &result) == B_OK
				&& result.FindMessage("items", &items) == B_OK) {

				BAutolock locker(&fLock);
				package->ClearUserRatings();

				int32 index = 0;
				while (true) {
					BString name;
					name << index++;

					BMessage item;
					if (items.FindMessage(name, &item) != B_OK)
						break;

					BString code;
					if (item.FindString("code", &code) != B_OK) {
						HDERROR("corrupt user rating at index %" B_PRIi32,
							index);
						continue;
					}

					BString user;
					BMessage userInfo;
					if (item.FindMessage("user", &userInfo) != B_OK
							|| userInfo.FindString("nickname", &user) != B_OK) {
						HDERROR("ignored user rating [%s] without a user "
							"nickname", code.String());
						continue;
					}

					// Extract basic info, all items are optional
					BString languageCode;
					BString comment;
					double rating;
					item.FindString("naturalLanguageCode", &languageCode);
					item.FindString("comment", &comment);
					if (item.FindDouble("rating", &rating) != B_OK)
						rating = -1;
					if (comment.Length() == 0 && rating == -1) {
						HDERROR("rating [%s] has no comment or rating so will"
							" be ignored", code.String());
						continue;
					}

					// For which version of the package was the rating?
					BString major = "?";
					BString minor = "?";
					BString micro = "";
					double revision = -1;
					BString architectureCode = "";
					BMessage version;
					if (item.FindMessage("pkgVersion", &version) == B_OK) {
						version.FindString("major", &major);
						version.FindString("minor", &minor);
						version.FindString("micro", &micro);
						version.FindDouble("revision", &revision);
						version.FindString("architectureCode",
							&architectureCode);
					}
					BString versionString = major;
					versionString << ".";
					versionString << minor;
					if (!micro.IsEmpty()) {
						versionString << ".";
						versionString << micro;
					}
					if (revision > 0) {
						versionString << "-";
						versionString << (int) revision;
					}

					if (!architectureCode.IsEmpty()) {
						versionString << " " << STR_MDASH << " ";
						versionString << architectureCode;
					}

					double createTimestamp;
					item.FindDouble("createTimestamp", &createTimestamp);

					// Add the rating to the PackageInfo
					UserRating userRating = UserRating(UserInfo(user), rating,
						comment, languageCode, versionString,
						(uint64) createTimestamp);
					package->AddUserRating(userRating);
					HDDEBUG("rating [%s] retrieved from server", code.String());
				}
				HDDEBUG("did retrieve %" B_PRIi32 " user ratings for [%s]",
						index - 1, packageName.String());
			} else {
				_MaybeLogJsonRpcError(info, "retrieve user ratings");
			}
		} else
			HDERROR("unable to retrieve user ratings");
	}

	if ((flags & POPULATE_SCREEN_SHOTS) != 0) {
		ScreenshotInfoList screenshotInfos;
		{
			BAutolock locker(&fLock);
			screenshotInfos = package->ScreenshotInfos();
			package->ClearScreenshots();
		}
		for (int i = 0; i < screenshotInfos.CountItems(); i++) {
			const ScreenshotInfo& info = screenshotInfos.ItemAtFast(i);
			_PopulatePackageScreenshot(package, info, 320, false);
		}
	}
}


void
Model::_PopulatePackageChangelog(const PackageInfoRef& package)
{
	BMessage info;
	BString packageName;

	{
		BAutolock locker(&fLock);
		packageName = package->Name();
	}

	status_t status = fWebAppInterface.GetChangelog(packageName, info);

	if (status == B_OK) {
		// Parse message
		BMessage result;
		BString content;
		if (info.FindMessage("result", &result) == B_OK) {
			if (result.FindString("content", &content) == B_OK
				&& 0 != content.Length()) {
				BAutolock locker(&fLock);
				package->SetChangelog(content);
				HDDEBUG("changelog populated for [%s]", packageName.String());
			} else
				HDDEBUG("no changelog present for [%s]", packageName.String());
		} else
			_MaybeLogJsonRpcError(info, "populate package changelog");
	} else {
		HDERROR("unable to obtain the changelog for the package [%s]",
			packageName.String());
	}
}


static void
model_remove_key_for_user(const BString& nickname)
{
	if (nickname.IsEmpty())
		return;
	BKeyStore keyStore;
	BPasswordKey key;
	BString passwordIdentifier = BString(KEY_STORE_IDENTIFIER_PREFIX)
		<< nickname;
	status_t result = keyStore.GetKey(kHaikuDepotKeyring, B_KEY_TYPE_PASSWORD,
			passwordIdentifier, key);

	switch (result) {
		case B_OK:
			result = keyStore.RemoveKey(kHaikuDepotKeyring, key);
			if (result != B_OK) {
				HDERROR("error occurred when removing password for nickname "
					"[%s] : %s", nickname.String(), strerror(result));
			}
			break;
		case B_ENTRY_NOT_FOUND:
			return;
		default:
			HDERROR("error occurred when finding password for nickname "
				"[%s] : %s", nickname.String(), strerror(result));
			break;
	}
}


void
Model::SetNickname(BString nickname)
{
	BString password;
	BString existingNickname = Nickname();

	// this happens when the user is logging out.  Best to remove the password
	// stored for the existing user since it is no longer required.

	if (!existingNickname.IsEmpty() && nickname.IsEmpty())
		model_remove_key_for_user(existingNickname);

	if (nickname.Length() > 0) {
		BPasswordKey key;
		BKeyStore keyStore;
		BString passwordIdentifier = BString(KEY_STORE_IDENTIFIER_PREFIX)
			<< nickname;
		if (keyStore.GetKey(kHaikuDepotKeyring, B_KEY_TYPE_PASSWORD,
				passwordIdentifier, key) == B_OK) {
			password = key.Password();
		}
		if (password.IsEmpty())
			nickname = "";
	}

	SetAuthorization(nickname, password, false);
}


const BString&
Model::Nickname() const
{
	return fWebAppInterface.Nickname();
}


void
Model::SetAuthorization(const BString& nickname, const BString& passwordClear,
	bool storePassword)
{
	BString existingNickname = Nickname();

	if (storePassword) {
		// no point continuing to store the password for the previous user.

		if (!existingNickname.IsEmpty())
			model_remove_key_for_user(existingNickname);

		// adding a key that is already there does not seem to override the
		// existing key so the old key needs to be removed first.

		if (!nickname.IsEmpty())
			model_remove_key_for_user(nickname);

		if (!nickname.IsEmpty() && !passwordClear.IsEmpty()) {
			BString keyIdentifier = BString(KEY_STORE_IDENTIFIER_PREFIX)
				<< nickname;
			BPasswordKey key(passwordClear, B_KEY_PURPOSE_WEB, keyIdentifier);
			BKeyStore keyStore;
			keyStore.AddKeyring(kHaikuDepotKeyring);
			keyStore.AddKey(kHaikuDepotKeyring, key);
		}
	}

	BAutolock locker(&fLock);
	fWebAppInterface.SetAuthorization(UserCredentials(nickname, passwordClear));

	if (nickname != existingNickname)
		_NotifyAuthorizationChanged();
}


/*! When bulk repository data comes down from the server, it will
    arrive as a json.gz payload.  This is stored locally as a cache
    and this method will provide the on-disk storage location for
    this file.
*/

status_t
Model::DumpExportRepositoryDataPath(BPath& path)
{
	BString leaf;
	leaf.SetToFormat("repository-all_%s.json.gz",
		Language()->PreferredLanguage()->Code());
	return StorageUtils::LocalWorkingFilesPath(leaf, path);
}


/*! When the system downloads reference data (eg; categories) from the server
    then the downloaded data is stored and cached at the path defined by this
    method.
*/

status_t
Model::DumpExportReferenceDataPath(BPath& path)
{
	BString leaf;
	leaf.SetToFormat("reference-all_%s.json.gz",
		Language()->PreferredLanguage()->Code());
	return StorageUtils::LocalWorkingFilesPath(leaf, path);
}


status_t
Model::IconTarPath(BPath& path) const
{
	return StorageUtils::LocalWorkingFilesPath("pkgicon-all.tar", path);
}


status_t
Model::DumpExportPkgDataPath(BPath& path,
	const BString& repositorySourceCode)
{
	BString leaf;
	leaf.SetToFormat("pkg-all-%s-%s.json.gz", repositorySourceCode.String(),
		Language()->PreferredLanguage()->Code());
	return StorageUtils::LocalWorkingFilesPath(leaf, path);
}


void
Model::_PopulatePackageScreenshot(const PackageInfoRef& package,
	const ScreenshotInfo& info, int32 scaledWidth, bool fromCacheOnly)
{
	// See if there is a cached screenshot
	BFile screenshotFile;
	BPath screenshotCachePath;

	status_t result = StorageUtils::LocalWorkingDirectoryPath(
		"Screenshots", screenshotCachePath);

	if (result != B_OK) {
		HDERROR("unable to get the screenshot dir - unable to proceed");
		return;
	}

	bool fileExists = false;
	BString screenshotName(info.Code());
	screenshotName << "@" << scaledWidth;
	screenshotName << ".png";
	time_t modifiedTime;
	if (screenshotCachePath.Append(screenshotName) == B_OK) {
		// Try opening the file in read-only mode, which will fail if its
		// not a file or does not exist.
		fileExists = screenshotFile.SetTo(screenshotCachePath.Path(),
			B_READ_ONLY) == B_OK;
		if (fileExists)
			screenshotFile.GetModificationTime(&modifiedTime);
	}

	if (fileExists) {
		time_t now;
		time(&now);
		if (fromCacheOnly || now - modifiedTime < 60 * 60) {
			// Cache file is recent enough, just use it and return.
			BitmapRef bitmapRef(new(std::nothrow)SharedBitmap(screenshotFile),
				true);
			BAutolock locker(&fLock);
			package->AddScreenshot(bitmapRef);
			return;
		}
	}

	if (fromCacheOnly)
		return;

	// Retrieve screenshot from web-app
	BMallocIO buffer;

	int32 scaledHeight = scaledWidth * info.Height() / info.Width();

	status_t status = fWebAppInterface.RetrieveScreenshot(info.Code(),
		scaledWidth, scaledHeight, &buffer);
	if (status == B_OK) {
		BitmapRef bitmapRef(new(std::nothrow)SharedBitmap(buffer), true);
		BAutolock locker(&fLock);
		package->AddScreenshot(bitmapRef);
		locker.Unlock();
		if (screenshotFile.SetTo(screenshotCachePath.Path(),
				B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE) == B_OK) {
			screenshotFile.Write(buffer.Buffer(), buffer.BufferLength());
		}
	} else {
		HDERROR("Failed to retrieve screenshot for code '%s' "
			"at %" B_PRIi32 "x%" B_PRIi32 ".", info.Code().String(),
			scaledWidth, scaledHeight);
	}
}


// #pragma mark - listener notification methods


void
Model::_NotifyAuthorizationChanged()
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--) {
		const ModelListenerRef& listener = fListeners.ItemAtFast(i);
		if (listener.Get() != NULL)
			listener->AuthorizationChanged();
	}
}


void
Model::_NotifyCategoryListChanged()
{
	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--) {
		const ModelListenerRef& listener = fListeners.ItemAtFast(i);
		if (listener.Get() != NULL)
			listener->CategoryListChanged();
	}
}


void
Model::_MaybeLogJsonRpcError(const BMessage &responsePayload,
	const char *sourceDescription) const
{
	BMessage error;
	BString errorMessage;
	double errorCode;

	if (responsePayload.FindMessage("error", &error) == B_OK
		&& error.FindString("message", &errorMessage) == B_OK
		&& error.FindDouble("code", &errorCode) == B_OK) {
		HDERROR("[%s] --> error : [%s] (%f)", sourceDescription,
			errorMessage.String(), errorCode);
	} else
		HDERROR("[%s] --> an undefined error has occurred", sourceDescription);
}


// #pragma mark - Rating Stabilities


int32
Model::CountRatingStabilities() const
{
	return fRatingStabilities.size();
}


RatingStabilityRef
Model::RatingStabilityByCode(BString& code) const
{
	std::vector<RatingStabilityRef>::const_iterator it;
	for (it = fRatingStabilities.begin(); it != fRatingStabilities.end();
			it++) {
		RatingStabilityRef aRatingStability = *it;
		if (aRatingStability->Code() == code)
			return aRatingStability;
	}
	return RatingStabilityRef();
}


RatingStabilityRef
Model::RatingStabilityAtIndex(int32 index) const
{
	return fRatingStabilities[index];
}


void
Model::AddRatingStabilities(std::vector<RatingStabilityRef>& values)
{
	std::vector<RatingStabilityRef>::const_iterator it;
	for (it = values.begin(); it != values.end(); it++)
		_AddRatingStability(*it);
}


void
Model::_AddRatingStability(const RatingStabilityRef& value)
{
	std::vector<RatingStabilityRef>::const_iterator itInsertionPtConst
		= std::lower_bound(
			fRatingStabilities.begin(),
			fRatingStabilities.end(),
			value,
			&IsRatingStabilityBefore);
	std::vector<RatingStabilityRef>::iterator itInsertionPt =
		fRatingStabilities.begin()
			+ (itInsertionPtConst - fRatingStabilities.begin());

	if (itInsertionPt != fRatingStabilities.end()
		&& (*itInsertionPt)->Code() == value->Code()) {
		itInsertionPt = fRatingStabilities.erase(itInsertionPt);
			// replace the one with the same code.
	}

	fRatingStabilities.insert(itInsertionPt, value);
}


// #pragma mark - Categories


int32
Model::CountCategories() const
{
	return fCategories.size();
}


CategoryRef
Model::CategoryByCode(BString& code) const
{
	std::vector<CategoryRef>::const_iterator it;
	for (it = fCategories.begin(); it != fCategories.end(); it++) {
		CategoryRef aCategory = *it;
		if (aCategory->Code() == code)
			return aCategory;
	}
	return CategoryRef();
}


CategoryRef
Model::CategoryAtIndex(int32 index) const
{
	return fCategories[index];
}


void
Model::AddCategories(std::vector<CategoryRef>& values)
{
	std::vector<CategoryRef>::iterator it;
	for (it = values.begin(); it != values.end(); it++)
		_AddCategory(*it);
	_NotifyCategoryListChanged();
}

/*! This will insert the category in order.
 */

void
Model::_AddCategory(const CategoryRef& category)
{
	std::vector<CategoryRef>::const_iterator itInsertionPtConst
		= std::lower_bound(
			fCategories.begin(),
			fCategories.end(),
			category,
			&IsPackageCategoryBefore);
	std::vector<CategoryRef>::iterator itInsertionPt =
		fCategories.begin() + (itInsertionPtConst - fCategories.begin());

	if (itInsertionPt != fCategories.end()
		&& (*itInsertionPt)->Code() == category->Code()) {
		itInsertionPt = fCategories.erase(itInsertionPt);
			// replace the one with the same code.
	}

	fCategories.insert(itInsertionPt, category);
}
