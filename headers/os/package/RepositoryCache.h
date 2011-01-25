/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__REPOSITORY_CACHE_H_
#define _PACKAGE__REPOSITORY_CACHE_H_


#include <Entry.h>
#include <String.h>

#include <package/RepositoryHeader.h>


namespace BPackageKit {


//class RepositoryHeader;


class BRepositoryCache {
public:
								BRepositoryCache();
								BRepositoryCache(const BEntry& entry);
	virtual						~BRepositoryCache();

			status_t			SetTo(const BEntry& entry);
			status_t			InitCheck() const;

			const BRepositoryHeader&	Header() const;
			const BEntry&		Entry() const;
			bool				IsUserSpecific() const;

			void				SetIsUserSpecific(bool isUserSpecific);

private:
			status_t			fInitStatus;

			BEntry				fEntry;
			BRepositoryHeader	fHeader;
			bool				fIsUserSpecific;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REPOSITORY_CACHE_H_
