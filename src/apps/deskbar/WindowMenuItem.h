/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//	individual windows of an application
//	item for WindowMenu, sub of TeamMenuItem
//	all DB positions

#ifndef WINDOWMENUITEM_H
#define WINDOWMENUITEM_H

#include <MenuItem.h>
#include <String.h>


class BBitmap;


class TWindowMenuItem : public BMenuItem {
	public:
		TWindowMenuItem(const char *title, int32 id, bool mini,
			bool currentWorkSpace, bool dragging = false);

		void ExpandedItem(bool state);
		void SetTo(const char *title, int32 id, bool mini,
			bool currentWorkSpace, bool dragging = false);
		int32 ID();
		void SetRequireUpdate();
		bool RequiresUpdate();
		bool ChangedState();

	virtual	void	SetLabel(const char* string);

	protected:
		void Initialize(const char *title);
		virtual	void GetContentSize(float *width, float *height);
		virtual void DrawContent();
		virtual status_t Invoke(BMessage *message = NULL);
		virtual void Draw();

	private: 
		int32			fID;
		bool			fMini;
		bool			fCurrentWorkSpace;
		const BBitmap*	fBitmap;
		float			fTitleWidth;
		float			fTitleAscent;
		float			fTitleDescent;
		bool			fDragging;
		bool			fExpanded;
		bool			fRequireUpdate;
		bool			fModified;
		BString			fFullTitle;
};

/****************************************************************************
** WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING **
**                                                                         **
**                          DANGER, WILL ROBINSON!                         **
**                                                                         **
** The rest of the interfaces contained here are part of BeOS's            **
**                                                                         **
**                     >> PRIVATE NOT FOR PUBLIC USE <<                    **
**                                                                         **
**                                                       implementation.   **
**                                                                         **
** These interfaces              WILL CHANGE        in future releases.    **
** If you use them, your app     WILL BREAK         at some future time.   **
**                                                                         **
** (And yes, this does mean that binaries built from OpenTracker will not  **
** be compatible with some future releases of the OS.  When that happens,  **
** we will provide an updated version of this file to keep compatibility.) **
**                                                                         **
** WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING **
****************************************************************************/

// from interface_defs.h
struct window_info {
	team_id		team;
    int32   	id;	  		  /* window's token */

	int32		thread;
	int32		client_token;
	int32		client_port;
	uint32		workspaces;

	int32		layer;
    uint32	  	w_type;    	  /* B_TITLED_WINDOW, etc. */
    uint32      flags;	  	  /* B_WILL_FLOAT, etc. */
	int32		window_left;
	int32		window_top;
	int32		window_right;
	int32		window_bottom;
	int32		show_hide_level;
	bool		is_mini;
	char		name[1];
};

// from interface_misc.h
enum window_action {
	B_MINIMIZE_WINDOW,
	B_BRING_TO_FRONT
};

// from interface_misc.h
void		do_window_action(int32 window_id, int32 action, 
							 BRect zoomRect, bool zoom);
window_info	*get_window_info(int32 a_token);
int32		*get_token_list(team_id app, int32 *count);
void do_minimize_team(BRect zoomRect, team_id team, bool zoom);
void do_bring_to_front_team(BRect zoomRect, team_id app, bool zoom);


#endif /* WINDOWMENUITEM_H */
