/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef DEPOT_INFO_H
#define DEPOT_INFO_H

#include <vector>

#include <String.h>
#include <Referenceable.h>

#include "PackageInfo.h"


class DepotInfoBuilder;


/*!	Instances of this class should not be created directly; instead use the
	DepotInfoBuilder class as a builder-constructor.
*/
class DepotInfo : public BReferenceable {
friend class DepotInfoBuilder;

public:
								DepotInfo();
								DepotInfo(const BString& name);
								DepotInfo(const DepotInfo& other);

			bool				operator==(const DepotInfo& other) const;
			bool				operator!=(const DepotInfo& other) const;

			const BString&		Name() const;
			const BString&		Identifier() const;
			const BString&		WebAppRepositoryCode() const;
			const BString&		WebAppRepositorySourceCode() const;

private:
			void				SetIdentifier(const BString& value);
			void				SetWebAppRepositoryCode(const BString& code);
			void				SetWebAppRepositorySourceCode(const BString& code);

private:
			BString				fName;
			BString				fIdentifier;
			BString				fWebAppRepositoryCode;
			BString				fWebAppRepositorySourceCode;
};


typedef BReference<DepotInfo> DepotInfoRef;


class DepotInfoBuilder
{
public:
								DepotInfoBuilder();
								DepotInfoBuilder(const DepotInfoRef& value);
	virtual						~DepotInfoBuilder();

			DepotInfoRef		BuildRef() const;

			DepotInfoBuilder&	WithName(const BString& value);
			DepotInfoBuilder&	WithIdentifier(const BString& value);
			DepotInfoBuilder&	WithWebAppRepositoryCode(const BString& value);
			DepotInfoBuilder&	WithWebAppRepositorySourceCode(const BString& value);

private:
			void				_InitFromSource();
			void				_Init(const DepotInfo* value);

private:
			DepotInfoRef		fSource;
			BString				fName;
			BString				fIdentifier;
			BString				fWebAppRepositoryCode;
			BString				fWebAppRepositorySourceCode;
};


#endif // DEPOT_INFO_H
