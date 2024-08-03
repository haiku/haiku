/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2014, Axel Dörfler <axeld@pinc-software.de>.
 * Copyright 2016-2024, Andrew Lindesay <apl@lindesay.co.nz>.
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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Model"


#define KEY_STORE_IDENTIFIER_PREFIX "hds.password."
	// this prefix is added before the nickname in the keystore
	// so that HDS username/password pairs can be identified.

static const char* kHaikuDepotKeyring = "HaikuDepot";


ModelListener::~ModelListener()
{
}


// #pragma mark - Model


Model::Model()
	:
	fPreferredLanguage(LanguageRef(new Language(LANGUAGE_DEFAULT_ID, "English", true), true)),
	fDepots(),
	fCategories(),
	fPackageListViewMode(PROMINENT),
	fCanShareAnonymousUsageData(false)
{

	// setup the language into a default state with a hard-coded language so
	// that there is a language at all.
	fLanguageRepository = new LanguageRepository();
	fLanguageRepository->AddLanguage(fPreferredLanguage);

	fPackageFilterModel = new PackageFilterModel();
	fPackageScreenshotRepository = new PackageScreenshotRepository(
		PackageScreenshotRepositoryListenerRef(this),
		&fWebAppInterface);
}


Model::~Model()
{
	delete fPackageFilterModel;
	delete fPackageScreenshotRepository;
	delete fLanguageRepository;
}


LanguageRepository*
Model::Languages()
{
	return fLanguageRepository;
}


PackageFilterModel*
Model::PackageFilter()
{
	return fPackageFilterModel;
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
	status_t result = StorageUtils::IconTarPath(tarPath);
	if (result == B_OK)
		result = fPackageIconRepository.Init(tarPath);
	return result;
}


PackageScreenshotRepository*
Model::GetPackageScreenshotRepository()
{
	return fPackageScreenshotRepository;
}


void
Model::AddListener(const ModelListenerRef& listener)
{
	fListeners.push_back(listener);
}


LanguageRef
Model::PreferredLanguage() const
{
	return fPreferredLanguage;
}


void
Model::SetPreferredLanguage(LanguageRef value)
{
	if (value.IsSet()) {
		if (fPreferredLanguage != value)
			fPreferredLanguage = value;
	}
}


// TODO; part of a wider change; cope with the package being in more than one
// depot
PackageInfoRef
Model::PackageForName(const BString& name)
{
	std::vector<DepotInfoRef>::iterator it;
	for (it = fDepots.begin(); it != fDepots.end(); it++) {
		DepotInfoRef depotInfoRef = *it;
		PackageInfoRef packageInfoRef = depotInfoRef->PackageByName(name);
		if (packageInfoRef.Get() != NULL)
			return packageInfoRef;
	}
	return PackageInfoRef();
}


void
Model::MergeOrAddDepot(const DepotInfoRef& depot)
{
	BString depotName = depot->Name();
	for(uint32 i = 0; i < fDepots.size(); i++) {
		if (fDepots[i]->Name() == depotName) {
			DepotInfoRef ersatzDepot(new DepotInfo(*(fDepots[i].Get())), true);
			ersatzDepot->SyncPackagesFromDepot(depot);
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
	GetPackageIconRepository().Clear();
	fDepots.clear();
	fPopulatedPackageNames.MakeEmpty();
}


void
Model::SetStateForPackagesByName(BStringList& packageNames, PackageState state)
{
	for (int32 i = 0; i < packageNames.CountStrings(); i++) {
		BString packageName = packageNames.StringAt(i);
		PackageInfoRef packageInfo = PackageForName(packageName);

		if (packageInfo.IsSet()) {
			packageInfo->SetState(state);
			HDINFO("did update package [%s] with state [%s]",
				packageName.String(), package_state_to_string(state));
		}
		else {
			HDINFO("was unable to find package [%s] so was not possible to set"
				" the state to [%s]", packageName.String(),
				package_state_to_string(state));
		}
	}
}


void
Model::SetPackageListViewMode(package_list_view_mode mode)
{
	fPackageListViewMode = mode;
}


void
Model::SetCanShareAnonymousUsageData(bool value)
{
	fCanShareAnonymousUsageData = value;
}


// #pragma mark - information retrieval

/*!	It may transpire that the package has no corresponding record on the
	server side because the repository is not represented in the server.
	In such a case, there is little point in communicating with the server
	only to hear back that the package does not exist.
*/

bool
Model::CanPopulatePackage(const PackageInfoRef& package)
{
	const BString& depotName = package->DepotName();

	if (depotName.IsEmpty())
		return false;

	const DepotInfoRef& depot = DepotForName(depotName);

	if (depot.Get() == NULL)
		return false;

	return !depot->WebAppRepositoryCode().IsEmpty();
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

	SetCredentials(nickname, password, false);
}


const BString&
Model::Nickname()
{
	return fWebAppInterface.Nickname();
}


void
Model::SetCredentials(const BString& nickname, const BString& passwordClear,
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
	fWebAppInterface.SetCredentials(UserCredentials(nickname, passwordClear));

	if (nickname != existingNickname)
		_NotifyAuthorizationChanged();
}


// #pragma mark - listener notification methods


void
Model::_NotifyAuthorizationChanged()
{
	std::vector<ModelListenerRef>::const_iterator it;
	for (it = fListeners.begin(); it != fListeners.end(); it++) {
		const ModelListenerRef& listener = *it;
		if (listener.IsSet())
			listener->AuthorizationChanged();
	}
}


void
Model::_NotifyCategoryListChanged()
{
	std::vector<ModelListenerRef>::const_iterator it;
	for (it = fListeners.begin(); it != fListeners.end(); it++) {
		const ModelListenerRef& listener = *it;
		if (listener.IsSet())
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


void
Model::ScreenshotCached(const ScreenshotCoordinate& coord)
{
	std::vector<ModelListenerRef>::const_iterator it;
	for (it = fListeners.begin(); it != fListeners.end(); it++) {
		const ModelListenerRef& listener = *it;
		if (listener.IsSet())
			listener->ScreenshotCached(coord);
	}
}
