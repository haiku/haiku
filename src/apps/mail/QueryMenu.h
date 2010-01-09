/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

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

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef _QUERY_MENU
#define _QUERY_MENU


#include <Locker.h>
#include <PopUpMenu.h>

#include <vector>

class BLooper;
class BQuery;
class BVolume;
class QHandler;
struct entry_ref;

using std::vector;

typedef vector<BQuery *> query_t;

class QueryMenu : public BPopUpMenu {
	friend class QHandler;
	
	public:
		QueryMenu(const char *title, bool popUp=false,
			bool radioMode = false, bool autoRename = false);
		virtual ~QueryMenu(void);
		
		virtual BPoint ScreenLocation(void);
		virtual status_t SetTargetForItems(BHandler *handler);
		virtual status_t SetTargetForItems(BMessenger messenger);
		status_t SetPredicate(const char *expr, BVolume *vol = NULL);
	
	protected:
		virtual void EntryCreated(const entry_ref &ref, ino_t node);
		virtual void EntryRemoved(ino_t node);
		virtual void RemoveEntries(void);
		
		BHandler *fTargetHandler;
		static BLooper *fQueryLooper;
		
	private:
		virtual void DoQueryMessage(BMessage *msg);
		static int32 query_thread(void *data);
		int32 QueryThread(void);
	
		BLocker fQueryLock;
//		BQuery *fQuery;
		query_t fQueries;
		QHandler *fQueryHandler;
		bool fCancelQuery;
		bool fPopUp;
		static int32 fMenuCount;
};

#endif // #ifndef _QUERY_MENU

