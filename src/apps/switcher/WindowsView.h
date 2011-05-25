/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef WINDOWS_VIEW_H
#define WINDOWS_VIEW_H


#include <GridView.h>


class GroupListView;


class WindowsView : public BGridView {
public:
								WindowsView(team_id team, uint32 location);
	virtual						~WindowsView();

protected:
	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			orientation			_Orientation(uint32 location);

private:
			GroupListView*		fListView;
};


#endif	// WINDOWS_VIEW_H
