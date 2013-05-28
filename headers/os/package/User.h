/*
 * Copyright 2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__USER_H_
#define _PACKAGE__USER_H_


#include <String.h>
#include <StringList.h>


namespace BPackageKit {


namespace BHPKG {
	struct BUserData;
}


class BUser {
public:
								BUser();
								BUser(const BHPKG::BUserData& userData);
								BUser(const BString& name,
									const BString& realName,
									const BString& home, const BString& shell,
									const BStringList& groups);
								~BUser();

			status_t			InitCheck() const;

			const BString&		Name() const;
			const BString&		RealName() const;
			const BString&		Home() const;
			const BString&		Shell() const;
			const BStringList&	Groups() const;

			status_t			SetTo(const BString& name,
									const BString& realName,
									const BString& home, const BString& shell,
									const BStringList& groups);

	static	bool				IsValidUserName(const char* name);

private:
			BString				fName;
			BString				fRealName;
			BString				fHome;
			BString				fShell;
			BStringList			fGroups;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__USER_H_
