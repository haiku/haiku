/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PANEL_WINDOW_H
#define PANEL_WINDOW_H


#include <Window.h>


class PanelWindow : public BWindow {
public:
								PanelWindow(uint32 location, uint32 which,
									team_id team);
	virtual						~PanelWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			BView*				_ViewFor(uint32 location, uint32 which,
									team_id team) const;
			void				_UpdateShowState(uint32 how);
			float				_Factor();

private:
			uint32				fLocation;
			int32				fShowState;
};


#endif	// PANEL_WINDOW_H
