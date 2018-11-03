/*
 * Copyright 2004-2018, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT license.
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
								ProbeView(entry_ref* ref,
									const char* attribute = NULL,
									const BMessage* settings = NULL);
	virtual						~ProbeView();

	virtual	void				DetachedFromWindow();
	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();
	virtual	void				WindowActivated(bool active);
	virtual	void				MessageReceived(BMessage* message);

			void				AddSaveMenuItems(BMenu* menu, int32 index);
			void				AddPrintMenuItems(BMenu* menu, int32 index);
			void				AddViewAsMenuItems();

			bool				QuitRequested();

			DataEditor&			Editor() { return fEditor; }

private:
			void				_UpdateAttributesMenu(BMenu* menu);
			void				_UpdateSelectionMenuItems(int64 start,
									int64 end);
			void				_UpdateBookmarkMenuItems();
			void				_AddBookmark(off_t position);
			void				_RemoveTypeEditor();
			void				_SetTypeEditor(int32 index);
			void				_CheckClipboard();
			status_t			_PageSetup();
			void				_Print();
			status_t			_Save();

private:
			DataEditor			fEditor;
			EditorLooper*		fEditorLooper;
			HeaderView*			fHeaderView;
			DataView*			fDataView;
			BScrollView*		fScrollView;
			BMenuItem*			fPasteMenuItem;
			BMenuItem*			fUndoMenuItem;
			BMenuItem*			fRedoMenuItem;
			BMenuItem*			fNativeMenuItem;
			BMenuItem*			fSwappedMenuItem;
			BMenuItem*			fSaveMenuItem;
			BMessage*			fPrintSettings;
			BMenu*				fBookmarkMenu;
			BView*				fTypeView;

			BMenuItem*			fFindAgainMenuItem;
			const uint8*		fLastSearch;
			size_t				fLastSearchSize;
};


#endif	/* PROBE_VIEW_H */
