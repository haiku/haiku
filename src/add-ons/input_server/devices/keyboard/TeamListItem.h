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
#include <Roster.h>
#include <String.h>


extern bool gLocalizedNamePreferred;


class TeamListItem : public BListItem  {
public:
								TeamListItem(team_info& info);
	virtual						~TeamListItem();

	virtual	void				DrawItem(BView* owner, BRect frame,
									bool complete = false);
	virtual	void				Update(BView* owner, const BFont* font);

			void				CacheLocalizedName();

	const	team_info*			GetInfo();
	const	BBitmap*			LargeIcon() { return &fLargeIcon; };
	const	BPath*				Path() { return &fPath; };
	const	char*				AppSignature() { return fAppInfo.signature; };

			bool				IsSystemServer();
			bool				IsApplication() const;

			bool				Found() const { return fFound; }
			void				SetFound(bool found) { fFound = found; }

			void				SetRefusingToQuit(bool refusing);
			bool				IsRefusingToQuit();

	static	int32				MinimalHeight();

private:
			team_info			fTeamInfo;
			app_info			fAppInfo;
			BBitmap				fMiniIcon;
			BBitmap				fLargeIcon;
			BPath				fPath;
			BString				fLocalizedName;
			bool				fFound;
			bool				fRefusingToQuit;
};


#endif	// TEAM_LIST_ITEM_H
