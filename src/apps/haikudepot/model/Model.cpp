/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2014, Axel Dörfler <axeld@pinc-software.de>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "Model.h"

#include <algorithm>
#include <ctime>

#include <stdarg.h>
#include <stdint.h>
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
#include "LocaleUtils.h"
#include "Logger.h"
#include "PackageUtils.h"
#include "StorageUtils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Model"


static const uint32 kChangeMaskAll = UINT32_MAX;
	// covers all of the possible values.


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
	fCanShareAnonymousUsageData(false),
	fWebApp(WebAppInterfaceRef(new WebAppInterface(UserCredentials()), true)),
	fLanguages(LocaleUtils::WellKnownLanguages())
{
	fIconRepository = PackageIconRepositoryRef(new PackageIconDefaultRepository(), true);

	fFilterSpecification = PackageFilterSpecificationBuilder().BuildRef();
	fFilter = PackageFilterFactory::CreateFilter(fFilterSpecification);

	fPackageScreenshotRepository
		= new PackageScreenshotRepository(PackageScreenshotRepositoryListenerRef(this), this);
}


Model::~Model()
{
	delete fPackageScreenshotRepository;
}


const std::vector<LanguageRef>
Model::Languages() const
{
	BAutolock locker(&fLock);
	return fLanguages;
}


void
Model::SetLanguagesAndPreferred(const std::vector<LanguageRef>& value, const LanguageRef& preferred)
{
	if (value.empty())
		HDFATAL("unable to set the languages to an empty list");

	if (std::find(value.begin(), value.end(), preferred) == value.end())
		HDFATAL("the preferred language is not in the list of languages");

	BAutolock locker(&fLock);
	fLanguages = value;
	std::sort(fLanguages.begin(), fLanguages.end(), IsLanguageRefLess);
	SetPreferredLanguage(preferred);

	HDINFO("did configure %" B_PRIu32 " languages with default [%s]",
		static_cast<uint32>(fLanguages.size()), preferred->ID());
}


void
Model::SetFilterSpecification(const PackageFilterSpecificationRef& value)
{
	BAutolock locker(&fLock);
	if (value != fFilterSpecification) {
		fFilterSpecification = value;
		fFilter = PackageFilterFactory::CreateFilter(value);
		_NotifyPackageFilterChanged();
	}
}


const PackageFilterSpecificationRef
Model::FilterSpecification() const
{
	BAutolock locker(&fLock);
	return fFilterSpecification;
}


PackageFilterRef
Model::Filter() const
{
	BAutolock locker(&fLock);
	return fFilter;
}


PackageIconRepositoryRef
Model::IconRepository()
{
	BAutolock locker(&fLock);
	return fIconRepository;
}


void
Model::SetIconRepository(PackageIconRepositoryRef value)
{
	BAutolock locker(&fLock);
	fIconRepository = value;
	_NotifyIconsChanged();
}


void
Model::_NotifyIconsChanged()
{
	std::vector<ModelListenerRef>::const_iterator it;
	for (it = fListeners.begin(); it != fListeners.end(); it++) {
		const ModelListenerRef& listener = *it;
		if (listener.IsSet())
			listener->IconsChanged();
	}
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


void
Model::AddPackageListener(const PackageInfoListenerRef& packageListener)
{
	fPackageListeners.push_back(packageListener);
}


/*!	This method will take the event info data stored in the supplied message
	and turn it back into an instance of `PackageInfoEvents`. It is done in
	this way because the lookup of the packages within each `PackageInfoEvent`
	has to come from the model.
*/
status_t
Model::DearchiveInfoEvents(const BMessage* message, PackageInfoEvents& packageInfoEvents) const
{
	BAutolock locker(&fLock);
	int32 i = 0;
	BMessage eventMessage;
	BString packageName;
	uint32 changeMask;

	while (message->FindMessage(kPackageInfoEventsKey, i, &eventMessage) == B_OK) {
		status_t result = B_OK;

		if (result == B_OK)
			result = eventMessage.FindString(kPackageInfoPackageNameKey, &packageName);

		if (result == B_OK)
			result = eventMessage.FindUInt32(kPackageInfoChangesKey, &changeMask);

		if (result == B_OK) {
			PackageInfoRef package = PackageForName(packageName);

			if (package.IsSet()) {
				PackageInfoEvent event(package, changeMask);
				packageInfoEvents.AddEvent(event);
			}
		} else {
			HDFATAL("broken event info found processing events");
				// should not occur as the data assembly is entirely inside the application
		}

		i++;
	}

	return B_OK;
}


const LanguageRef
Model::PreferredLanguage() const
{
	BAutolock locker(&fLock);
	return fPreferredLanguage;
}


void
Model::SetPreferredLanguage(const LanguageRef& value)
{
	if (value.IsSet()) {
		BAutolock locker(&fLock);
		fPreferredLanguage = value;
	}
}


void
Model::SetDepots(const DepotInfoRef& depot)
{
	BAutolock locker(&fLock);
	std::vector<DepotInfoRef> depots;
	depots.push_back(depot);
	SetDepots(depots);
}


void
Model::SetDepots(const std::vector<DepotInfoRef>& depots)
{
	BAutolock locker(&fLock);
	fDepots.clear();

	std::vector<DepotInfoRef>::const_iterator it;

	for (it = depots.begin(); it != depots.end(); it++) {
		DepotInfoRef depot = *it;

		if (!depot.IsSet()) {
			HDFATAL("attempt to add an unset depot");
		} else {
			if (depot->Name().IsEmpty())
				HDFATAL("depot has no name");

			if (depot->Identifier().IsEmpty())
				HDFATAL("depot has no identifier");

			fDepots[depot->Name()] = depot;
		}
	}
}


const std::vector<DepotInfoRef>
Model::Depots() const
{
	BAutolock locker(&fLock);
	std::vector<DepotInfoRef> result;
	std::map<BString, DepotInfoRef>::const_iterator it;

	for (it = fDepots.begin(); it != fDepots.end(); it++)
		result.push_back(it->second);

	return result;
}


const DepotInfoRef
Model::DepotForName(const BString& name) const
{
	BAutolock locker(&fLock);
	std::map<BString, DepotInfoRef>::const_iterator it = fDepots.find(name);
	if (it != fDepots.end())
		return it->second;
	return DepotInfoRef();
}


const DepotInfoRef
Model::DepotForIdentifier(const BString& identifier) const
{
	BAutolock locker(&fLock);
	std::map<BString, DepotInfoRef>::const_iterator it;

	for (it = fDepots.begin(); it != fDepots.end(); it++) {
		if (it->second->Identifier() == identifier)
			return it->second;
	}

	return DepotInfoRef();
}


const std::vector<PackageInfoRef>
Model::FilteredPackages() const
{
	BAutolock locker(&fLock);
	std::map<BString, PackageInfoRef>::const_iterator it;
	std::vector<PackageInfoRef> result;

	for (it = fPackages.begin(); it != fPackages.end(); it++) {
		PackageInfoRef package = it->second;
		if (fFilter->AcceptsPackage(package))
			result.push_back(package);
	}

	return result;
}


const std::vector<PackageInfoRef>
Model::Packages() const
{
	BAutolock locker(&fLock);
	std::map<BString, PackageInfoRef>::const_iterator it;
	std::vector<PackageInfoRef> result;

	for (it = fPackages.begin(); it != fPackages.end(); it++)
		result.push_back(it->second);

	return result;
}


uint32
Model::_ChangeDiff(const PackageInfoRef& package)
{
	uint32 changeMask = kChangeMaskAll;
	const PackageInfoRef existingPackage = PackageForName(package->Name());

	if (existingPackage.IsSet())
		changeMask = package->ChangeMask(*(existingPackage.Get()));

	return changeMask;
}


void
Model::AddPackage(const PackageInfoRef& package)
{
	if (!package.IsSet())
		HDFATAL("attempt to add an unset package");
	BAutolock locker(&fLock);
	uint32 changeMask = _ChangeDiff(package);
	fPackages[package->Name()] = package;
	_NotifyPackageChange(PackageInfoEvent(package, changeMask));
}


void
Model::AddPackages(const std::vector<PackageInfoRef>& packages)
{
	if (packages.empty())
		return;

	BAutolock locker(&fLock);
	PackageInfoEvents events;
	std::vector<PackageInfoRef>::const_iterator it;

	for (it = packages.begin(); it != packages.end(); it++) {
		PackageInfoRef package = *it;
		events.AddEvent(PackageInfoEvent(package, _ChangeDiff(package)));
		fPackages[package->Name()] = package;
	}

	_NotifyPackageChanges(events);
}


void
Model::AddPackagesWithChange(const std::vector<PackageInfoRef>& packages, uint32 changeMask)
{
	if (packages.empty())
		return;

	BAutolock locker(&fLock);
	PackageInfoEvents events;

	std::vector<PackageInfoRef>::const_iterator it;
	for (it = packages.begin(); it != packages.end(); it++) {
		PackageInfoRef package = *it;
		events.AddEvent(PackageInfoEvent(package, changeMask));
		fPackages[package->Name()] = package;
	}

	_NotifyPackageChanges(events);
}


const PackageInfoRef
Model::PackageForName(const BString& name) const
{
	BAutolock locker(&fLock);
	std::map<BString, PackageInfoRef>::const_iterator it = fPackages.find(name);
	if (it != fPackages.end())
		return it->second;
	return PackageInfoRef();
}


bool
Model::HasPackage(const BString& packageName) const
{
	BAutolock locker(&fLock);
	if (fPackages.find(packageName) != fPackages.end())
		return true;
	return false;
}


bool
Model::HasAnyProminentPackages()
{
	BAutolock locker(&fLock);
	std::map<BString, PackageInfoRef>::iterator it;

	for (it = fPackages.begin(); it != fPackages.end(); it++) {
		const PackageInfoRef package = it->second;

		if (package.IsSet()) {
			const PackageClassificationInfoRef classificationInfo
				= package->PackageClassificationInfo();

			if (classificationInfo.IsSet()) {
				if (classificationInfo->IsProminent())
					return true;
			}
		}
	}

	return false;
}


void
Model::Clear()
{
	BAutolock locker(&fLock);
	fIconRepository = PackageIconRepositoryRef(new PackageIconDefaultRepository(), true);
	fDepots.clear();
	fPackages.clear();
}


package_list_view_mode
Model::PackageListViewMode() const
{
	BAutolock locker(&fLock);
	return fPackageListViewMode;
}


void
Model::SetPackageListViewMode(package_list_view_mode mode)
{
	BAutolock locker(&fLock);
	fPackageListViewMode = mode;
}


bool
Model::CanShareAnonymousUsageData() const
{
	BAutolock locker(&fLock);
	return fCanShareAnonymousUsageData;
}


void
Model::SetCanShareAnonymousUsageData(bool value)
{
	BAutolock locker(&fLock);
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
	const BString depotName = PackageUtils::DepotName(package);
	const DepotInfoRef& depot = DepotForName(depotName);

	if (!depot.IsSet())
		return false;

	return !depot->WebAppRepositoryCode().IsEmpty();
}


WebAppInterfaceRef
Model::WebApp()
{
	BAutolock locker(&fLock);
	return fWebApp;
}


const BString&
Model::Nickname()
{
	BAutolock locker(&fLock);
	return fWebApp->Nickname();
}


void
Model::SetCredentials(const UserCredentials& credentials)
{
	BAutolock locker(&fLock);
	const BString existingNickname = Nickname();
	fWebApp = WebAppInterfaceRef(new WebAppInterface(credentials));
	if (credentials.Nickname() != existingNickname)
		_NotifyAuthorizationChanged();
}


// #pragma mark - listener notification methods


/*!	Assumes that the class is locked.
 */
void
Model::_NotifyPackageFilterChanged()
{
	std::vector<ModelListenerRef>::const_iterator it;
	for (it = fListeners.begin(); it != fListeners.end(); it++) {
		const ModelListenerRef& listener = *it;
		if (listener.IsSet())
			listener->PackageFilterChanged();
	}
}


/*!	Assumes that the class is locked.
 */
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
Model::_NotifyPackageChange(const PackageInfoEvent& event)
{
	std::vector<PackageInfoListenerRef>::const_iterator it;
	for (it = fPackageListeners.begin(); it != fPackageListeners.end(); it++) {
		const PackageInfoListenerRef& listener = *it;
		if (listener.IsSet())
			listener->PackagesChanged(PackageInfoEvents(event));
	}
}


// TODO: future work to optimize how this is conveyed to the listener in one go.
void
Model::_NotifyPackageChanges(const PackageInfoEvents& events)
{
	if (events.IsEmpty())
		return;

	std::vector<PackageInfoListenerRef>::const_iterator it;
	for (it = fPackageListeners.begin(); it != fPackageListeners.end(); it++) {
		const PackageInfoListenerRef& listener = *it;
		if (listener.IsSet())
			listener->PackagesChanged(events);
	}
}


void
Model::_MaybeLogJsonRpcError(const BMessage& responsePayload, const char* sourceDescription) const
{
	BMessage error;
	BString errorMessage;
	double errorCode;

	if (responsePayload.FindMessage("error", &error) == B_OK
		&& error.FindString("message", &errorMessage) == B_OK
		&& error.FindDouble("code", &errorCode) == B_OK) {
		HDERROR("[%s] --> error : [%s] (%f)", sourceDescription, errorMessage.String(), errorCode);
	} else {
		HDERROR("[%s] --> an undefined error has occurred", sourceDescription);
	}
}


// #pragma mark - Rating Stabilities


const std::vector<RatingStabilityRef>
Model::RatingStabilities() const
{
	BAutolock locker(&fLock);
	return fRatingStabilities;
}


void
Model::SetRatingStabilities(const std::vector<RatingStabilityRef> value)
{
	BAutolock locker(&fLock);
	fRatingStabilities = value;
	std::sort(fRatingStabilities.begin(), fRatingStabilities.end(), IsRatingStabilityRefLess);
}


// #pragma mark - Categories


bool
Model::HasCategories()
{
	BAutolock locker(&fLock);
	return !fCategories.empty();
}


const std::vector<CategoryRef>
Model::Categories() const
{
	BAutolock locker(&fLock);
	return fCategories;
}


void
Model::SetCategories(const std::vector<CategoryRef> value)
{
	BAutolock locker(&fLock);
	fCategories = value;
	std::sort(fCategories.begin(), fCategories.end(), IsPackageCategoryRefLess);
	_NotifyCategoryListChanged();
}


void
Model::ScreenshotCached(const ScreenshotCoordinate& coord)
{
	BAutolock locker(&fLock);
	std::vector<ModelListenerRef>::const_iterator it;
	for (it = fListeners.begin(); it != fListeners.end(); it++) {
		const ModelListenerRef& listener = *it;
		if (listener.IsSet())
			listener->ScreenshotCached(coord);
	}
}
