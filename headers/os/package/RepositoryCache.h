/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__REPOSITORY_CACHE_H_
#define _HAIKU__PACKAGE__REPOSITORY_CACHE_H_


#include <Entry.h>
#include <String.h>


class BEntry;


namespace Haiku {

namespace Package {


//class RepositoryHeader;


class RepositoryCache {
public:
								RepositoryCache();
								RepositoryCache(const BEntry& entry);
	virtual						~RepositoryCache();

			status_t			SetTo(const BEntry& entry);
			status_t			InitCheck() const;

//			const RepositoryHeader*	Header() const;
			const BEntry&		Entry() const;
			bool				IsUserSpecific() const;

			void				SetIsUserSpecific(bool isUserSpecific);

private:
			status_t			fInitStatus;

			BEntry				fEntry;
//			RepositoryHeader*	fHeader;
			bool				fIsUserSpecific;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__REPOSITORY_CACHE_H_
