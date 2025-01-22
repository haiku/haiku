/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_INFO_H
#define USER_INFO_H


#include <String.h>


class UserInfo {
public:
								UserInfo();
								UserInfo(const BString& nickName);
								UserInfo(const UserInfo& other);

			bool				operator==(const UserInfo& other) const;
			bool				operator!=(const UserInfo& other) const;

			const BString&		NickName() const
									{ return fNickName; }

private:
			BString				fNickName;
};


#endif // USER_INFO_H
