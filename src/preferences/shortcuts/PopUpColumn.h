/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Josef Gajdusek
 */
#ifndef POPUPCOLUMN_H
#define POPUPCOLUMN_H

#include <ColumnTypes.h>

class BPopUpMenu;

class PopUpColumn : public BStringColumn {
public:
						PopUpColumn(BPopUpMenu* menu, const char* name,
								float width, float minWidth, float maxWidth,
								uint32 truncate, bool editable = false,
								bool cycle = false, int cycleInit = 0,
								alignment align = B_ALIGN_LEFT);
	virtual				~PopUpColumn();

			void		MouseDown(BColumnListView* parent, BRow* row,
							BField* field, BRect fieldRect, BPoint point,
							uint32 buttons);

private:
			bool		fEditable;
			bool		fCycle;
			int			fCycleInit;
			BPopUpMenu*	fMenu;
};

#endif	// POPUPCOLUMN_H
