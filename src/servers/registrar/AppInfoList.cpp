//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		AppInfoList.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	A helper class for TRoster. A list of RosterAppInfos.
//------------------------------------------------------------------------------

#include "AppInfoList.h"
#include "RosterAppInfo.h"

// constructor
AppInfoList::AppInfoList()
		   : fInfos()
{
}

// constructor
AppInfoList::~AppInfoList()
{
	// delete all infos
	for (int32 i = 0; RosterAppInfo *info = InfoAt(i); i++)
		delete info;
}

// AddInfo
bool
AppInfoList::AddInfo(RosterAppInfo *info, int32 index)
{
	bool result = false;
	if (info) {
		if (index < 0)
			result = fInfos.AddItem(info);
		else
			result = fInfos.AddItem(info, index);
	}
	return result;
}

// RemoveInfo
RosterAppInfo *
AppInfoList::RemoveInfo(int32 index)
{
	return (RosterAppInfo*)fInfos.RemoveItem(index);
}

// RemoveInfo
bool
AppInfoList::RemoveInfo(RosterAppInfo *info)
{
	return fInfos.RemoveItem(info);
}

// IndexOf
int32
AppInfoList::IndexOf(RosterAppInfo *info) const
{
	return fInfos.IndexOf(info);
}

// IndexOf
int32
AppInfoList::IndexOf(const char *signature) const
{
	for (int32 i = 0; RosterAppInfo *info = InfoAt(i); i++) {
		if (!strcmp(info->signature, signature))
			return i;
	}
	return -1;
}

// IndexOf
int32
AppInfoList::IndexOf(team_id team) const
{
	for (int32 i = 0; RosterAppInfo *info = InfoAt(i); i++) {
		if (info->team == team)
			return i;
	}
	return -1;
}

// IndexOf
int32
AppInfoList::IndexOf(const entry_ref *ref) const
{
	for (int32 i = 0; RosterAppInfo *info = InfoAt(i); i++) {
		if (info->ref == *ref)
			return i;
	}
	return -1;
}

// IndexOfToken
int32
AppInfoList::IndexOfToken(uint32 token) const
{
	for (int32 i = 0; RosterAppInfo *info = InfoAt(i); i++) {
		if (info->token == token)
			return i;
	}
	return -1;
}

// InfoAt
RosterAppInfo *
AppInfoList::InfoAt(int32 index) const
{
	return (RosterAppInfo*)fInfos.ItemAt(index);
}

// InfoFor
RosterAppInfo *
AppInfoList::InfoFor(const char *signature) const
{
	return InfoAt(IndexOf(signature));
}

// InfoFor
RosterAppInfo *
AppInfoList::InfoFor(team_id team) const
{
	return InfoAt(IndexOf(team));
}

// InfoFor
RosterAppInfo *
AppInfoList::InfoFor(const entry_ref *ref) const
{
	return InfoAt(IndexOf(ref));
}

// InfoForToken
RosterAppInfo *
AppInfoList::InfoForToken(uint32 token) const
{
	return InfoAt(IndexOfToken(token));
}

// CountInfos
int32
AppInfoList::CountInfos() const
{
	return fInfos.CountItems();
}

