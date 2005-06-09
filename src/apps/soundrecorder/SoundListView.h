/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Inspired by SoundCapture from Be newsletter (Media Kit Basics: Consumers and Producers)
 */

#ifndef SOUNDLISTVIEW_H
#define SOUNDLISTVIEW_H

#include <ListView.h>

class SoundListView : public BListView {
public:
	SoundListView(const BRect & area, const char * name, uint32 resize);
	virtual	~SoundListView();

	virtual void Draw(BRect updateRect);
	virtual	void AttachedToWindow();
};


#include <ListItem.h>

class SoundListItem : public BStringItem {
public:
		SoundListItem(const BEntry & entry, bool isTemp);
virtual	~SoundListItem();

		BEntry & Entry() { return fEntry; }
		bool IsTemp() { return fIsTemp; }
		void SetTemp(bool isTemp) { fIsTemp = isTemp; }		
private:
		BEntry fEntry;
		bool fIsTemp;
};


#endif	/* SOUNDLISTVIEW_H */

