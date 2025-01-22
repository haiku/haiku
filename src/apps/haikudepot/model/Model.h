/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <map>

#include <Locker.h>

#include "AbstractProcess.h"
#include "DepotInfo.h"
#include "PackageFilter.h"
#include "PackageFilterSpecification.h"
#include "PackageIconDefaultRepository.h"
#include "PackageInfo.h"
#include "PackageInfoListener.h"
#include "PackageScreenshotRepository.h"
#include "RatingStability.h"
#include "ScreenshotCoordinate.h"
#include "WebAppInterface.h"


class BFile;
class BMessage;
class BPath;


typedef enum package_list_view_mode {
	PROMINENT,
	ALL
} package_list_view_mode;


class ModelListener : public BReferenceable {
public:
	virtual						~ModelListener();

	virtual	void				AuthorizationChanged() = 0;
	virtual void				CategoryListChanged() = 0;
	virtual void				ScreenshotCached(const ScreenshotCoordinate& coordinate) = 0;
	virtual void				IconsChanged() = 0;
	virtual void				PackageFilterChanged() = 0;
};


typedef BReference<ModelListener> ModelListenerRef;


class PackageConsumer {
public:
	virtual	bool				ConsumePackage(
									const PackageInfoRef& packageInfoRef,
									void* context) = 0;
};


class Model : public PackageScreenshotRepositoryListener {
public:
								Model();
	virtual						~Model();

			void				Clear();

			void				AddListener(const ModelListenerRef& listener);
			void				AddPackageListener(const PackageInfoListenerRef& packageListener);

			status_t			DearchiveInfoEvents(const BMessage* message,
									PackageInfoEvents& packageInfoEvents) const;

			PackageScreenshotRepository*
								GetPackageScreenshotRepository();

			void				SetFilterSpecification(const PackageFilterSpecificationRef& value);
	const 	PackageFilterSpecificationRef
								FilterSpecification() const;
			PackageFilterRef	Filter() const;

			PackageIconRepositoryRef
								IconRepository();
			void				SetIconRepository(PackageIconRepositoryRef value);

			const std::vector<PackageInfoRef>
								Packages() const;
			const std::vector<PackageInfoRef>
								FilteredPackages() const;
			void				AddPackage(const PackageInfoRef& package);
			void				AddPackages(const std::vector<PackageInfoRef>& packages);
			void				AddPackagesWithChange(const std::vector<PackageInfoRef>& packages,
									uint32 changesMask);
			const PackageInfoRef
								PackageForName(const BString& name) const;
			bool				HasPackage(const BString& packageName) const;
			bool				HasAnyProminentPackages();

	const	LanguageRef			PreferredLanguage() const;
			void				SetPreferredLanguage(const LanguageRef& value);
	const	std::vector<LanguageRef>
								Languages() const;
			void				SetLanguagesAndPreferred(const std::vector<LanguageRef>& value,
									const LanguageRef& preferred);

			void				SetDepots(const DepotInfoRef& depot);
			void				SetDepots(const std::vector<DepotInfoRef>& depots);
	const	std::vector<DepotInfoRef>
								Depots() const;
	const DepotInfoRef			DepotForName(const BString& name) const;
	const DepotInfoRef			DepotForIdentifier(const BString& identifier) const;

	const	std::vector<CategoryRef>
								Categories() const;
			void				SetCategories(const std::vector<CategoryRef> value);
			bool				HasCategories();

	const	std::vector<RatingStabilityRef>
								RatingStabilities() const;
			void				SetRatingStabilities(const std::vector<RatingStabilityRef> value);

			void				SetPackageListViewMode(
									package_list_view_mode mode);
			package_list_view_mode
								PackageListViewMode() const;

			void				SetCanShareAnonymousUsageData(bool value);
			bool				CanShareAnonymousUsageData() const;

			bool				CanPopulatePackage(const PackageInfoRef& package);

			void				SetCredentials(const UserCredentials& credentials);
	const	BString&			Nickname();
			WebAppInterfaceRef	WebApp();

			// PackageScreenshotRepositoryListener
    virtual	void				ScreenshotCached(const ScreenshotCoordinate& coord);


private:
			uint32				_ChangeDiff(const PackageInfoRef& package);

			void				_AddRatingStability(
									const RatingStabilityRef& value);

			void				_MaybeLogJsonRpcError(
									const BMessage &responsePayload,
									const char *sourceDescription) const;

			void				_NotifyPackageFilterChanged();
			void				_NotifyIconsChanged();
			void				_NotifyAuthorizationChanged();
			void				_NotifyCategoryListChanged();
			void				_NotifyPackageChange(const PackageInfoEvent& event);
			void				_NotifyPackageChanges(const PackageInfoEvents& events);

private:
	mutable	BLocker				fLock;

			LanguageRef			fPreferredLanguage;

			std::map<BString, DepotInfoRef>
								fDepots;
			std::map<BString, PackageInfoRef>
								fPackages;

			std::vector<CategoryRef>
								fCategories;
			std::vector<RatingStabilityRef>
								fRatingStabilities;

			package_list_view_mode
								fPackageListViewMode;

			bool				fCanShareAnonymousUsageData;

			WebAppInterfaceRef	fWebApp;

			PackageFilterSpecificationRef
								fFilterSpecification;
			PackageFilterRef	fFilter;

			std::vector<LanguageRef>
								fLanguages;
			PackageIconRepositoryRef
								fIconRepository;
			PackageScreenshotRepository*
								fPackageScreenshotRepository;

			std::vector<ModelListenerRef>
								fListeners;

			std::vector<PackageInfoListenerRef>
								fPackageListeners;
};


#endif // MODEL_H
