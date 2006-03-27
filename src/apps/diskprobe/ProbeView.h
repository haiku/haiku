/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PROBE_VIEW_H
#define PROBE_VIEW_H


#include "DataEditor.h"

#include <View.h>
#include <String.h>
#include <Path.h>


class BScrollView;
class BMenuItem;
class BMenu;

class HeaderView;
class DataView;
class EditorLooper;


class ProbeView : public BView {
	public:
		ProbeView(BRect rect, entry_ref *ref, const char *attribute = NULL,
			const BMessage *settings = NULL);
		virtual ~ProbeView();

		virtual void DetachedFromWindow();
		virtual void AttachedToWindow();
		virtual void AllAttached();
		virtual void WindowActivated(bool active);
		virtual void MessageReceived(BMessage *message);

		void AddSaveMenuItems(BMenu *menu, int32 index);
		void AddPrintMenuItems(BMenu *menu, int32 index);

		void UpdateSizeLimits();
		bool QuitRequested();

		DataEditor &Editor() { return fEditor; }

	private:
		void UpdateAttributesMenu(BMenu *menu);
		void UpdateSelectionMenuItems(int64 start, int64 end);
		void UpdateBookmarkMenuItems();
		void AddBookmark(off_t position);
		void CheckClipboard();
		status_t PageSetup();
		void Print();
		status_t Save();

		DataEditor		fEditor;
		EditorLooper	*fEditorLooper;
		HeaderView		*fHeaderView;
		DataView		*fDataView;
		BScrollView		*fScrollView;
		BMenuItem		*fPasteMenuItem;
		BMenuItem		*fUndoMenuItem, *fRedoMenuItem;
		BMenuItem		*fNativeMenuItem, *fSwappedMenuItem;
		BMenuItem		*fSaveMenuItem;
		BMessage		*fPrintSettings;
		BMenu			*fBookmarkMenu;

		BMenuItem		*fFindAgainMenuItem;
		const uint8		*fLastSearch;
		size_t			fLastSearchSize;
};

#endif	/* PROBE_VIEW_H */
