/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__ROSTER_H_
#define _HAIKU__PACKAGE__ROSTER_H_


#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <SupportDefs.h>


template <class T> class BObjectList;


namespace Haiku {

namespace Package {


struct RepositoryConfigVisitor {
	virtual ~RepositoryConfigVisitor()
	{
	}

	virtual status_t operator()(const BEntry& entry) = 0;
};


class RepositoryCache;
class RepositoryConfig;


class Roster {
public:
								Roster();
								~Roster();

			status_t			GetCommonRepositoryCachePath(BPath* path,
									bool create = false) const;
			status_t			GetUserRepositoryCachePath(BPath* path,
									bool create = false) const;

			status_t			GetCommonRepositoryConfigPath(BPath* path,
									bool create = false) const;
			status_t			GetUserRepositoryConfigPath(BPath* path,
									bool create = false) const;

			status_t			GetRepositoryNames(BObjectList<BString>& names);

			status_t			VisitCommonRepositoryConfigs(
									RepositoryConfigVisitor& visitor);
			status_t			VisitUserRepositoryConfigs(
									RepositoryConfigVisitor& visitor);

			status_t			GetRepositoryCache(const BString& name,
									RepositoryCache* repositoryCache);
			status_t			GetRepositoryConfig(const BString& name,
									RepositoryConfig* repositoryConfig);
private:
			status_t			_GetRepositoryPath(BPath* path, bool create,
									directory_which whichDir) const;
			status_t			_VisitRepositoryConfigs(const BPath& path,
									RepositoryConfigVisitor& visitor);

};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__ROSTER_H_
