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
//	File Name:		AppInfoList.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	A helper class for TRoster. A list of RosterAppInfos.
//------------------------------------------------------------------------------

#ifndef APP_INFO_LIST_H
#define APP_INFO_LIST_H

#include <List.h>
#include <OS.h>

class entry_ref;

class RosterAppInfo;

class AppInfoList {
public:
	AppInfoList();
	virtual ~AppInfoList();

	bool AddInfo(RosterAppInfo *info, int32 index = -1);

	RosterAppInfo *RemoveInfo(int32 index);
	bool RemoveInfo(RosterAppInfo *info);

	int32 IndexOf(RosterAppInfo *info) const;
	int32 IndexOf(const char *signature) const;
	int32 IndexOf(team_id team) const;
	int32 IndexOf(const entry_ref *ref) const;
	int32 IndexOfToken(uint32 token) const;

	RosterAppInfo *InfoAt(int32 index) const;
	RosterAppInfo *InfoFor(const char *signature) const;
	RosterAppInfo *InfoFor(team_id team) const;
	RosterAppInfo *InfoFor(const entry_ref *ref) const;
	RosterAppInfo *InfoForToken(uint32 token) const;

	int32 CountInfos() const;

private:
	BList	fInfos;
};

#endif	// APP_INFO_LIST_H
