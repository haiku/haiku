/*
 * Copyright 2004-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
#ifndef TEAM_LIST_ITEM_H
#define TEAM_LIST_ITEM_H


#include <Bitmap.h>
#include <ListItem.h>
#include <Path.h>
#include <String.h>


class TeamListItem : public BListItem  {
public:
								TeamListItem(team_info& info);
	virtual						~TeamListItem();

	virtual	void				DrawItem(BView* owner, BRect frame,
									bool complete = false);
	virtual	void				Update(BView* owner, const BFont* font);

	const	team_info*			GetInfo();
	const	BBitmap*			LargeIcon() { return &fLargeIcon; };
	const	BPath*				Path() { return &fPath; };
	const	BString*			AppSignature() { return &fAppSignature; };

			bool				IsSystemServer();
			bool				IsApplication();

			bool				Found() const { return fFound; }
			void				SetFound(bool found) { fFound = found; }

	static	int32				MinimalHeight();

private:
			team_info			fInfo;
			BBitmap				fIcon, fLargeIcon;
			BPath				fPath;
			BString				fAppSignature;
			bool				fFound;
};


#endif	// TEAM_LIST_ITEM_H
