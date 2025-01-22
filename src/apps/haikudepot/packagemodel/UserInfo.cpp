/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "UserInfo.h"


UserInfo::UserInfo()
	:
	fNickName()
{
}


UserInfo::UserInfo(const BString& nickName)
	:
	fNickName(nickName)
{
}


UserInfo::UserInfo(const UserInfo& other)
	:
	fNickName(other.fNickName)
{
}


bool
UserInfo::operator==(const UserInfo& other) const
{
	return fNickName == other.fNickName;
}


bool
UserInfo::operator!=(const UserInfo& other) const
{
	return !(*this == other);
}
