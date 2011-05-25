/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef APPLICATIONS_VIEW_H
#define APPLICATIONS_VIEW_H


#include <GroupView.h>
#include <Roster.h>


class ApplicationsView : public BGroupView {
public:
								ApplicationsView(uint32 location);
	virtual						~ApplicationsView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			void				_AddTeam(app_info& info);
};


#endif	// APPLICATIONS_VIEW_H
