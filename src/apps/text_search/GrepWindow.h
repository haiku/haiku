/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef GREP_WINDOW_H
#define GREP_WINDOW_H

#include <Catalog.h>
#include <InterfaceKit.h>
#include <FilePanel.h>
#include <Locale.h>

#include "Model.h"
#include "GrepListView.h"

class BMessageRunner;
class ChangesIterator;
class Grepper;

class GrepWindow : public BWindow {
public:
								GrepWindow(BMessage* message);
	virtual						~GrepWindow();

	virtual	void				FrameResized(float width, float height);
	virtual	void				FrameMoved(BPoint origin);
	virtual	void				MenusBeginning();
	virtual	void				MenusEnded();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Quit();

private:
			void				_InitRefsReceived(entry_ref* directory,
									BMessage* message);
			void				_SetWindowTitle();
			void				_CreateMenus();
			void				_UpdateMenus();
			void				_CreateViews();
			void				_LayoutViews();
			void				_TileIfMultipleWindows();

			void				_LoadPrefs();
			void				_SavePrefs();

			void				_StartNodeMonitoring();
			void				_StopNodeMonitoring();

			void				_OnStartCancel();
			void				_OnSearchFinished();
			void				_OnNodeMonitorEvent(BMessage* message);
			void				_OnNodeMonitorPulse();
			void				_OnReportFileName(BMessage* message);
			void				_OnReportResult(BMessage* message);
			void				_OnReportError(BMessage* message);
			void				_OnRecurseLinks();
			void				_OnRecurseDirs();
			void				_OnSkipDotDirs();
			void				_OnRegularExpression();
			void				_OnCaseSensitive();
			void				_OnTextOnly();
			void				_OnInvokeEditor();
			void				_OnCheckboxShowLines();
			void				_OnInvokeItem();
			void				_OnSearchText();
			void				_OnHistoryItem(BMessage* message);
			void				_OnTrimSelection();
			void				_OnCopyText();
			void				_OnSelectInTracker();
			void				_OnQuitNow();
			void				_OnFileDrop(BMessage* message);
			void				_OnRefsReceived(BMessage* message);
			void				_OnOpenPanel();
			void				_OnOpenPanelCancel();
			void				_OnSelectAll(BMessage* message);
			void				_OnNewWindow();
			void				_OnSetTargetToParent();

			void				_ModelChanged();
			bool				_OpenInEditor(const entry_ref& ref, int32 lineNum);
			void				_RemoveFolderListDuplicates(BList* folderList);
			status_t			_OpenFoldersInTracker(BList* folderList);
			bool				_AreAllFoldersOpenInTracker(BList* folderList);
			status_t			_SelectFilesInTracker(BList* folderList,
									BMessage* refsMessage);

private:
			BTextControl*		fSearchText;
			GrepListView*		fSearchResults;

			BMenuBar*			fMenuBar;
			BMenu*				fFileMenu;
			BMenuItem*			fNew;
			BMenuItem*			fOpen;
			BMenuItem*			fSetTargetToParent;
			BMenuItem*			fClose;
			BMenuItem*			fQuit;
			BMenu*				fActionMenu;
			BMenuItem*			fSelectAll;
			BMenuItem*			fSearch;
			BMenuItem*			fTrimSelection;
			BMenuItem*			fCopyText;
			BMenuItem*			fSelectInTracker;
			BMenuItem*			fOpenSelection;
			BMenu*				fPreferencesMenu;
			BMenuItem*			fRecurseLinks;
			BMenuItem*			fRecurseDirs;
			BMenuItem*			fSkipDotDirs;
			BMenuItem*			fCaseSensitive;
			BMenuItem*			fRegularExpression;
			BMenuItem*			fTextOnly;
			BMenuItem*			fInvokeEditor;
			BMenu*				fHistoryMenu;
			BMenu*				fEncodingMenu;
			BMenuItem*			fUTF8;
			BMenuItem*			fShiftJIS;
			BMenuItem*			fEUC;
			BMenuItem*			fJIS;

			BCheckBox*			fShowLinesCheckbox;
			BButton*			fButton;

			Grepper*			fGrepper;
			BString				fOldPattern;
			Model*				fModel;
			bigtime_t			fLastNodeMonitorEvent;
			ChangesIterator*	fChangesIterator;
			BMessageRunner*		fChangesPulse;


			BFilePanel*			fFilePanel;
};

#endif // GREP_WINDOW_H
