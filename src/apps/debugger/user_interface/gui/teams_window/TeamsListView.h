/*
 * Copyright 2009-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Philippe Houdoin
 */
#ifndef TEAMS_LIST_ITEM_H
#define TEAMS_LIST_ITEM_H


#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <OS.h>

class BBitmap;
class BMessageRunner;


// A field type displaying both a bitmap and a string so that the
// tree display looks nicer (both text and bitmap are indented)
class BBitmapStringField : public BStringField {
	typedef BStringField Inherited;
public:
								BBitmapStringField(BBitmap* bitmap,
									const char* string);
	virtual						~BBitmapStringField();

			void				SetBitmap(BBitmap* bitmap);
			const BBitmap*		Bitmap() const
									{ return fBitmap; }

private:
			BBitmap*			fBitmap;
};


// BColumn for TeamsListView which knows how to render
// a BBitmapStringField
class TeamsColumn : public BTitledColumn {
	typedef BTitledColumn Inherited;
public:
								TeamsColumn(const char* title,
									float width, float minWidth,
									float maxWidth, uint32 truncateMode,
									alignment align = B_ALIGN_LEFT);

	virtual	void				DrawField(BField* field, BRect rect,
									BView* parent);
	virtual float				GetPreferredWidth(BField* field, BView* parent) const;

	virtual	bool				AcceptsField(const BField* field) const;

	static	void				InitTextMargin(BView* parent);

private:
			uint32				fTruncateMode;
	static	float				sTextMargin;
};



class TeamRow : public BRow {
	typedef BRow Inherited;
public:
								TeamRow(team_info& teamInfo);
								TeamRow(team_id teamId);

public:
			team_id				TeamID() const 				{ return fTeamInfo.team; }

			bool				NeedsUpdate(team_info& info);

	virtual	void				SetEnabled(bool enabled) 	{ fEnabled = enabled; }
			bool				IsEnabled() const			{ return fEnabled; }

private:
			status_t			_SetTo(team_info& info);

private:
			bool				fEnabled;
			team_info			fTeamInfo;
};


class TeamsListView : public BColumnListView {
	typedef BColumnListView Inherited;
public:
								TeamsListView(BRect frame, const char* name);
	virtual 					~TeamsListView();

			TeamRow* 			FindTeamRow(team_id teamId);

protected:
	virtual void 				AttachedToWindow();
	virtual void 				DetachedFromWindow();

	virtual void 				MessageReceived(BMessage* message);

private:
			void 				_InitList();
			void 				_UpdateList();

private:
			BMessageRunner*		fUpdateRunner;
			team_id				fThisTeam;
};

static const uint32 kMsgUpdateTeamsList = 'uptl';

#endif	// TEAMS_LIST_VIEW_H
