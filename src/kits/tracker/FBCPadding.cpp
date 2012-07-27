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


#include <FilePanel.h>

#include "FilePanelPriv.h"
#include "RecentItems.h"


// FBC fluff, stick it here to not pollute real .cpp files

void BRecentItemsList::_r1() {}
void BRecentItemsList::_r2() {}
void BRecentItemsList::_r3() {}
void BRecentItemsList::_r4() {}
void BRecentItemsList::_r5() {}
void BRecentItemsList::_r6() {}
void BRecentItemsList::_r7() {}
void BRecentItemsList::_r8() {}
void BRecentItemsList::_r9() {}
void BRecentItemsList::_r10() {}
void BRecentFilesList::_r11() {}
void BRecentFilesList::_r12() {}
void BRecentFilesList::_r13() {}
void BRecentFilesList::_r14() {}
void BRecentFilesList::_r15() {}
void BRecentFilesList::_r16() {}
void BRecentFilesList::_r17() {}
void BRecentFilesList::_r18() {}
void BRecentFilesList::_r19() {}
void BRecentFilesList::_r110() {}
void BRecentFoldersList::_r21() {}
void BRecentFoldersList::_r22() {}
void BRecentFoldersList::_r23() {}
void BRecentFoldersList::_r24() {}
void BRecentFoldersList::_r25() {}
void BRecentFoldersList::_r26() {}
void BRecentFoldersList::_r27() {}
void BRecentFoldersList::_r28() {}
void BRecentFoldersList::_r29() {}
void BRecentFoldersList::_r210() {}
void BRecentAppsList::_r31() {}
void BRecentAppsList::_r32() {}
void BRecentAppsList::_r33() {}
void BRecentAppsList::_r34() {}
void BRecentAppsList::_r35() {}
void BRecentAppsList::_r36() {}
void BRecentAppsList::_r37() {}
void BRecentAppsList::_r38() {}
void BRecentAppsList::_r39() {}
void BRecentAppsList::_r310() {}

void BFilePanel::_ReservedFilePanel1() {}
void BFilePanel::_ReservedFilePanel2() {}
void BFilePanel::_ReservedFilePanel3() {}
void BFilePanel::_ReservedFilePanel4() {}
void BFilePanel::_ReservedFilePanel5() {}
void BFilePanel::_ReservedFilePanel6() {}
void BFilePanel::_ReservedFilePanel7() {}
void BFilePanel::_ReservedFilePanel8() {}

// deprecated cruft

#if __GNUC__ && __GNUC__ < 3 || __MWERKS__
extern "C" {

_EXPORT BFilePanel*
#if __GNUC__
__10BFilePanel15file_panel_modeP10BMessengerP9entry_refUlbP8BMessageP10BRefFilterT5T5
#elif __MWERKS__
__ct__10BFilePanelF15file_panel_modeP10BMessengerP9entry_refUlbP8BMessageP10BRefFilterbb
#endif
(void* self,
	file_panel_mode mode, BMessenger* target,
	entry_ref* ref, uint32 nodeFlavors, bool multipleSelection,
	BMessage* message, BRefFilter* filter, bool modal,
	bool hideWhenDone)
{
	return new (self) BFilePanel(mode, target, ref, nodeFlavors,
								 multipleSelection, message, filter, modal,
								 hideWhenDone);
}

_EXPORT void
#if __GNUC__
SetPanelDirectory__10BFilePanelP10BDirectory
#elif __MWERKS__
SetPanelDirectory__10BFilePanelFP10BDirectory
#endif
(BFilePanel* self, BDirectory* d)
{
	self->SetPanelDirectory(d);
}

_EXPORT void
#if __GNUC__
SetPanelDirectory__10BFilePanelP6BEntry
#elif __MWERKS__
SetPanelDirectory__10BFilePanelFP6BEntry
#endif
(BFilePanel* self, BEntry* e)
{
	self->SetPanelDirectory(e);
}

_EXPORT void
#if __GNUC__
SetPanelDirectory__10BFilePanelP9entry_ref
#elif __MWERKS__
SetPanelDirectory__10BFilePanelFP9entry_ref
#endif
(BFilePanel* self, entry_ref* r)
{
	self->SetPanelDirectory(r);
}
	
}
#endif
