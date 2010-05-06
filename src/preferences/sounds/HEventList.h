/*
 * Copyright 2003-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Jérôme Duval
 *		Oliver Ruiz Dorantes
 *		Atsushi Takamatsu
 */
#ifndef __HEVENTLIST_H__
#define __HEVENTLIST_H__


#include <ColumnListView.h>
#include <String.h>


enum {
	kEventColumn,
	kSoundColumn,
};


class HEventRow : public BRow {
public:
								HEventRow(const char* event_name,
									const char* path);
	virtual						~HEventRow();
		
			const char*			Name() const { return fName.String(); }
			const char*			Path() const { return fPath.String(); }
			void				Remove(const char* type);
			void				SetPath(const char* path);

private:
			BString				fName;
			BString				fPath;
};


enum {
	M_EVENT_CHANGED = 'SCAG'
};


class HEventList : public BColumnListView {
public:
								HEventList(const char* name = "EventList");
	virtual						~HEventList();
			void				RemoveAll();
			void				SetType(const char* type);
			void				SetPath(const char* path);

protected:
	virtual	void				SelectionChanged();

private:
			char*				fType;	
};


#endif	// __HEVENTLIST_H__
