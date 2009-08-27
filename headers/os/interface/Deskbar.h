/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_DESKBAR_H
#define	_DESKBAR_H


#include <Rect.h>

class BMessenger;
class BView;
struct entry_ref;


enum deskbar_location {
	B_DESKBAR_TOP,
	B_DESKBAR_BOTTOM,
	B_DESKBAR_LEFT_TOP,
	B_DESKBAR_RIGHT_TOP,
	B_DESKBAR_LEFT_BOTTOM,
	B_DESKBAR_RIGHT_BOTTOM
};

class BDeskbar {
public:
								BDeskbar();
								~BDeskbar();

			bool				IsRunning() const;

	// Location member functions
			BRect				Frame() const;
			deskbar_location	Location(bool* _isExpanded = NULL) const;
			status_t			SetLocation(deskbar_location location,
									bool expanded = false);
			bool				IsExpanded() const;
			status_t			Expand(bool expand);

	// Item querying member functions
			status_t			GetItemInfo(int32 id,
									const char** _name) const;
			status_t			GetItemInfo(const char* name,
									int32* _id) const;
			bool				HasItem(int32 id) const;
			bool				HasItem(const char* name) const;
			uint32				CountItems() const;

	// Item modification member functions
			status_t			AddItem(BView* archivableView,
									int32* _id = NULL);
			status_t			AddItem(entry_ref* addOn, int32* _id = NULL);
			status_t			RemoveItem(int32 id);
			status_t			RemoveItem(const char* name);

private:
			BMessenger*			fMessenger;
			uint32				_reserved[12];
};

#endif	// _DESKBAR_H
