/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef DEPOT_INFO_H
#define DEPOT_INFO_H

#include <vector>

#include <String.h>
#include <Referenceable.h>

#include "PackageInfo.h"


class DepotInfo : public BReferenceable {
public:
								DepotInfo();
								DepotInfo(const BString& name);
								DepotInfo(const DepotInfo& other);

			DepotInfo&			operator=(const DepotInfo& other);
			bool				operator==(const DepotInfo& other) const;
			bool				operator!=(const DepotInfo& other) const;

			const BString&		Name() const
									{ return fName; }

			int32				CountPackages() const;
			PackageInfoRef		PackageAtIndex(int32 index);
			void				AddPackage(PackageInfoRef& package);
			PackageInfoRef		PackageByName(const BString& packageName);
			bool				HasPackage(const BString& packageName);

			void				SyncPackagesFromDepot(
									const BReference<DepotInfo>& other);

			bool				HasAnyProminentPackages() const;

			void				SetURL(const BString& URL);
			const BString&		URL() const
									{ return fURL; }

			void				SetWebAppRepositoryCode(const BString& code);
			const BString&		WebAppRepositoryCode() const
									{ return fWebAppRepositoryCode; }

			void				SetWebAppRepositorySourceCode(
									const BString& code);
			const BString&		WebAppRepositorySourceCode() const
									{ return fWebAppRepositorySourceCode; }

private:
			BString				fName;
			std::vector<PackageInfoRef>
								fPackages;
			BString				fWebAppRepositoryCode;
			BString				fWebAppRepositorySourceCode;
			BString				fURL;
				// this is actually a unique identifier for the repository.
};


typedef BReference<DepotInfo> DepotInfoRef;


#endif // DEPOT_INFO_H
