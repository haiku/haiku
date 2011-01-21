/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__ROSTER_H_
#define _HAIKU__PACKAGE__ROSTER_H_


#include <Entry.h>
#include <Path.h>
#include <SupportDefs.h>


namespace Haiku {

namespace Package {


struct RepositoryConfigVisitor {
	virtual ~RepositoryConfigVisitor()
	{
	}

	virtual status_t operator()(const BEntry& entry) = 0;
};


class Roster {
public:
								Roster();
								~Roster();

			status_t			GetCommonRepositoryConfigPath(BPath* path,
									bool create = false) const;
			status_t			GetUserRepositoryConfigPath(BPath* path,
									bool create = false) const;

			status_t			VisitCommonRepositoryConfigs(
									RepositoryConfigVisitor& visitor);
			status_t			VisitUserRepositoryConfigs(
									RepositoryConfigVisitor& visitor);

private:
			status_t			_VisitRepositoryConfigs(const BPath& path,
									RepositoryConfigVisitor& visitor);

};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__ROSTER_H_
