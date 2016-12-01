/*
 * Copyright 2009-2016, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Rene Gollent
 *		Philippe Houdoin
 */
#ifndef TEAMS_LIST_ITEM_H
#define TEAMS_LIST_ITEM_H

#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <OS.h>

#include "TargetHost.h"
#include "TeamInfo.h"
#include "TeamsWindow.h"


class BBitmap;
class TargetHostInterface;


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
								TeamRow(TeamInfo* teamInfo);

public:
			team_id				TeamID() const
									{ return fTeamInfo.TeamID(); }

			bool				NeedsUpdate(TeamInfo* info);

	virtual	void				SetEnabled(bool enabled)
									{ fEnabled = enabled; }
			bool				IsEnabled() const
									{ return fEnabled; }

private:
			status_t			_SetTo(TeamInfo* info);

private:
			bool				fEnabled;
			TeamInfo			fTeamInfo;
};


class TeamsListView : public BColumnListView, public TargetHost::Listener,
	public TeamsWindow::Listener {
	typedef BColumnListView Inherited;
public:
								TeamsListView(const char* name);
	virtual 					~TeamsListView();

			TeamRow* 			FindTeamRow(team_id teamId);

			// TargetHost::Listener
	virtual	void				TeamAdded(TeamInfo* info);
	virtual	void				TeamRemoved(team_id team);
	virtual	void				TeamRenamed(TeamInfo* info);

			// TeamsWindow::Listener
	virtual	void				SelectedInterfaceChanged(
									TargetHostInterface* interface);

protected:
	virtual void 				AttachedToWindow();
	virtual void 				DetachedFromWindow();

	virtual void 				MessageReceived(BMessage* message);

private:
			void 				_InitList();
			void				_SetInterface(TargetHostInterface* interface);

private:
			TargetHostInterface* fInterface;
};


#endif	// TEAMS_LIST_VIEW_H
