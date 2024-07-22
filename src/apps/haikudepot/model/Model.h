/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MODEL_H
#define MODEL_H

#include <vector>

#include <Locker.h>

#include "AbstractProcess.h"
#include "DepotInfo.h"
#include "LanguageModel.h"
#include "PackageFilterModel.h"
#include "PackageIconTarRepository.h"
#include "PackageInfo.h"
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

			LanguageModel*		Language();
			PackageFilterModel*	PackageFilter();
			PackageIconRepository&
								GetPackageIconRepository();
			status_t			InitPackageIconRepository();
			PackageScreenshotRepository*
								GetPackageScreenshotRepository();

			BLocker*			Lock()
									{ return &fLock; }

			void				AddListener(const ModelListenerRef& listener);

			PackageInfoRef		PackageForName(const BString& name);

			void				MergeOrAddDepot(const DepotInfoRef& depot);
			bool				HasDepot(const BString& name) const;
			int32				CountDepots() const;
			DepotInfoRef		DepotAtIndex(int32 index) const;
			const DepotInfoRef	DepotForName(const BString& name) const;
			bool				HasAnyProminentPackages();

			void				Clear();

			int32				CountCategories() const;
			CategoryRef			CategoryByCode(BString& code) const;
			CategoryRef			CategoryAtIndex(int32 index) const;
			void				AddCategories(
									std::vector<CategoryRef>& values);

			int32				CountRatingStabilities() const;
			RatingStabilityRef	RatingStabilityByCode(BString& code) const;
			RatingStabilityRef	RatingStabilityAtIndex(int32 index) const;
			void				AddRatingStabilities(
									std::vector<RatingStabilityRef>& values);

			void				SetStateForPackagesByName(
									BStringList& packageNames,
									PackageState state);


			void				SetPackageListViewMode(
									package_list_view_mode mode);
			package_list_view_mode
								PackageListViewMode() const
									{ return fPackageListViewMode; }

			void				SetCanShareAnonymousUsageData(bool value);
			bool				CanShareAnonymousUsageData() const
									{ return fCanShareAnonymousUsageData; }

			// Retrieve package information
	static	const uint32		POPULATE_CACHED_RATING	= 1 << 0;
	static	const uint32		POPULATE_CACHED_ICON	= 1 << 1;
	static	const uint32		POPULATE_USER_RATINGS	= 1 << 2;
	static	const uint32		POPULATE_CHANGELOG		= 1 << 3;
	static	const uint32		POPULATE_CATEGORIES		= 1 << 4;
	static	const uint32		POPULATE_FORCE			= 1 << 5;

			bool				CanPopulatePackage(
									const PackageInfoRef& package);
			void				PopulatePackage(const PackageInfoRef& package,
									uint32 flags);

			void				SetNickname(BString nickname);
			const BString&		Nickname();
			void				SetCredentials(const BString& nickname,
									const BString& passwordClear,
									bool storePassword);

			WebAppInterface*    GetWebAppInterface()
									{ return &fWebAppInterface; }

			status_t			IconTarPath(BPath& path) const;
			status_t			DumpExportReferenceDataPath(BPath& path);
			status_t			DumpExportRepositoryDataPath(BPath& path);
			status_t			DumpExportPkgDataPath(BPath& path,
									const BString& repositorySourceCode);

			// PackageScreenshotRepositoryListener
    virtual	void				ScreenshotCached(const ScreenshotCoordinate& coord);

private:
			void				_AddCategory(const CategoryRef& category);

			void				_AddRatingStability(
									const RatingStabilityRef& value);

			void				_MaybeLogJsonRpcError(
									const BMessage &responsePayload,
									const char *sourceDescription) const;

	static	int32				_PopulateAllPackagesEntry(void* cookie);

			void				_PopulatePackageChangelog(
									const PackageInfoRef& package);

			void				_NotifyAuthorizationChanged();
			void				_NotifyCategoryListChanged();

private:
			BLocker				fLock;

			std::vector<DepotInfoRef>
								fDepots;
			std::vector<CategoryRef>
								fCategories;
			std::vector<RatingStabilityRef>
								fRatingStabilities;

			BStringList			fPopulatedPackageNames;

			package_list_view_mode
								fPackageListViewMode;

			bool				fCanShareAnonymousUsageData;

			WebAppInterface		fWebAppInterface;
			LanguageModel		fLanguageModel;
			PackageFilterModel*	fPackageFilterModel;
			PackageIconTarRepository
								fPackageIconRepository;
			PackageScreenshotRepository*
								fPackageScreenshotRepository;

			std::vector<ModelListenerRef>
								fListeners;
};


#endif // PACKAGE_INFO_H
