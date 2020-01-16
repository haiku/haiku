//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku
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

#include <algorithm>

#include <strings.h>

#include "AppInfoList.h"
#include "RosterAppInfo.h"

/*!
	\class AppInfoList
	\brief A list of RosterAppInfos.

	Features adding/removing of RosterAppInfos and method for finding
	infos by signature, team ID, entry_ref or token.
	The method It() returns an iterator, an instance of the basic
	AppInfoList::Iterator class.
*/


// constructor
/*!	\brief Creates an empty list.
*/
AppInfoList::AppInfoList()
		   : fInfos()
{
}

// destructor
/*!	\brief Frees all resources associated with this object.

	The RosterAppInfos the list contains are deleted.
*/
AppInfoList::~AppInfoList()
{
	// delete all infos
	MakeEmpty(true);
}

// AddInfo
/*!	\brief Adds a RosterAppInfos to the list.
	\param info The RosterAppInfo to be added
	\return \c true on success, false if \a info is \c NULL or there's not
			enough memory for this operation.
*/
bool
AppInfoList::AddInfo(RosterAppInfo *info)
{
	bool result = false;
	if (info)
		result = fInfos.AddItem(info);
	return result;
}

// RemoveInfo
/*!	\brief Removes a RosterAppInfos from the list.
	\param info The RosterAppInfo to be removed
	\return \c true on success, false if \a info was not in the list.
*/
bool
AppInfoList::RemoveInfo(RosterAppInfo *info)
{
	return fInfos.RemoveItem(info);
}

// MakeEmpty
/*!	\brief Removes all RosterAppInfos from the list.
*/
void
AppInfoList::MakeEmpty(bool deleteInfos)
{
	if (deleteInfos) {
		for (int32 i = 0; RosterAppInfo *info = InfoAt(i); i++)
			delete info;
	}

	fInfos.MakeEmpty();
}

// InfoFor
/*!	\brief Returns the RosterAppInfo with the supplied signature.

	If the list contains more than one RosterAppInfo with the given signature,
	it is undefined, which one is returned.

	\param signature The signature
	\return A RosterAppInfo with the supplied signature, or \c NULL, if
			\a signature is \c NULL or the list doesn't contain an info with
			this signature.
*/
RosterAppInfo *
AppInfoList::InfoFor(const char *signature) const
{
	return InfoAt(IndexOf(signature));
}

// InfoFor
/*!	\brief Returns the RosterAppInfo with the supplied team ID.
	\param team The team ID
	\return A RosterAppInfo with the supplied team ID, or \c NULL, if the list
			doesn't contain an info with this team ID.
*/
RosterAppInfo *
AppInfoList::InfoFor(team_id team) const
{
	return InfoAt(IndexOf(team));
}

// InfoFor
/*!	\brief Returns the RosterAppInfo with the supplied entry_ref.

	If the list contains more than one RosterAppInfo with the given entry_ref,
	it is undefined, which one is returned.

	\param ref The entry_ref
	\return A RosterAppInfo with the supplied entry_ref, or \c NULL, if
			\a ref is \c NULL or the list doesn't contain an info with
			this entry_ref.
*/
RosterAppInfo *
AppInfoList::InfoFor(const entry_ref *ref) const
{
	return InfoAt(IndexOf(ref));
}

// InfoForToken
/*!	\brief Returns the RosterAppInfo with the supplied token.

	If the list contains more than one RosterAppInfo with the given token,
	it is undefined, which one is returned.

	\param token The token
	\return A RosterAppInfo with the supplied token, or \c NULL, if the list
			doesn't contain an info with the token.
*/
RosterAppInfo *
AppInfoList::InfoForToken(uint32 token) const
{
	return InfoAt(IndexOfToken(token));
}

// CountInfos
/*!	\brief Returns the number of RosterAppInfos this list contains.
	\return The number of RosterAppInfos this list contains.
*/
int32
AppInfoList::CountInfos() const
{
	return fInfos.CountItems();
}

// It
/*!	\brief Returns a list iterator.
	\return A list iterator.
*/
AppInfoList::Iterator
AppInfoList::It()
{
	return Iterator(this, 0);
}

// Sort
/*!	\brief Sorts the infos in ascending order according to the given compare
		   function.
	\param lessFunc The compare function (less than) to be used.
*/
void
AppInfoList::Sort(
	bool (*lessFunc)(const RosterAppInfo *, const RosterAppInfo *))
{
	int32 count = CountInfos();
	if (count > 1) {
		RosterAppInfo **infos = (RosterAppInfo **)fInfos.Items();
		std::sort(infos, infos + count, lessFunc);
	}
}

// RemoveInfo
/*!	\brief Removes a RosterAppInfo at a given index.
	\param index The index of the info to be removed
	\return A pointer to the removed RosterAppInfo, or \c NULL, if the index
			is out of range.
*/
RosterAppInfo *
AppInfoList::RemoveInfo(int32 index)
{
	return (RosterAppInfo*)fInfos.RemoveItem(index);
}

// InfoAt
/*!	\brief Returns a RosterAppInfo at a given index.
	\param index The index of the info to be returned
	\return A pointer to the RosterAppInfo, or \c NULL, if the index
			is out of range.
*/
RosterAppInfo *
AppInfoList::InfoAt(int32 index) const
{
	return (RosterAppInfo*)fInfos.ItemAt(index);
}

// IndexOf
/*!	\brief Returns the list index of the supplied RosterAppInfo.
	\param info The RosterAppInfo in question
	\return The index of the supplied info, or -1, if \a info is \c NULL or not
			contained in the list.
*/
int32
AppInfoList::IndexOf(RosterAppInfo *info) const
{
	return fInfos.IndexOf(info);
}

// IndexOf
/*!	\brief Returns the list index of a RosterAppInfo with the supplied
		   signature.

	If the list contains more than one RosterAppInfo with the given signature,
	it is undefined, which one is returned.

	\param signature The signature
	\return The index of the found RosterAppInfo, or -1, if \a signature is
			\c NULL or the list doesn't contain an info with this signature.
*/
int32
AppInfoList::IndexOf(const char *signature) const
{
	if (signature) {
		for (int32 i = 0; RosterAppInfo *info = InfoAt(i); i++) {
			if (!strcasecmp(info->signature, signature))
				return i;
		}
	}
	return -1;
}

// IndexOf
/*!	\brief Returns the list index of a RosterAppInfo with the supplied
		   team ID.

	\param team The team ID
	\return The index of the found RosterAppInfo, or -1, if the list doesn't
			contain an info with this team ID.
*/
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
/*!	\brief Returns the list index of a RosterAppInfo with the supplied
		   entry_ref.

	If the list contains more than one RosterAppInfo with the given entry_ref,
	it is undefined, which one is returned.

	\param ref The entry_ref
	\return The index of the found RosterAppInfo, or -1, if \a ref is
			\c NULL or the list doesn't contain an info with this entry_ref.
*/
int32
AppInfoList::IndexOf(const entry_ref *ref) const
{
	if (ref) {
		// Dereference symlink if needed
		BEntry entry(ref, true);
		entry_ref realRef;
		if (entry.GetRef(&realRef) != B_OK)
			realRef = *ref;

		for (int32 i = 0; RosterAppInfo *info = InfoAt(i); i++) {
			if (info->ref == realRef)
				return i;
		}
	}
	return -1;
}

// IndexOfToken
/*!	\brief Returns the list index of a RosterAppInfo with the supplied
		   token.

	\param token The token
	\return The index of the found RosterAppInfo, or -1, if the list doesn't
			contain an info with this token.
*/
int32
AppInfoList::IndexOfToken(uint32 token) const
{
	for (int32 i = 0; RosterAppInfo *info = InfoAt(i); i++) {
		if (info->token == token)
			return i;
	}
	return -1;
}

