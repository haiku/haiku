/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "GrepWindow.h"

#include <ctype.h>
#include <errno.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Application.h>
#include <AppFileInfo.h>
#include <Alert.h>
#include <Clipboard.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <MimeType.h>
#include <Path.h>
#include <PathMonitor.h>
#include <Roster.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <UTF8.h>

#include "ChangesIterator.h"
#include "GlobalDefs.h"
#include "Grepper.h"
#include "InitialIterator.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GrepWindow"


using std::nothrow;

static const bigtime_t kChangesPulseInterval = 150000;

#define TRACE_NODE_MONITORING
#ifdef TRACE_NODE_MONITORING
# define TRACE_NM(x...) printf(x)
#else
# define TRACE_NM(x...)
#endif

//#define TRACE_FUNCTIONS
#ifdef TRACE_FUNCTIONS
	class FunctionTracer {
	public:
		FunctionTracer(const char* functionName)
			: fName(functionName)
		{
			printf("%s - enter\n", fName.String());
		}
		~FunctionTracer()
		{
			printf("%s - exit\n", fName.String());
		}
	private:
		BString	fName;
	};
# define CALLED()	FunctionTracer functionTracer(__PRETTY_FUNCTION__)
#else
# define CALLED()
#endif // TRACE_FUNCTIONS


GrepWindow::GrepWindow(BMessage* message)
	: BWindow(BRect(0, 0, 525, 430), NULL, B_DOCUMENT_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS),
	fSearchText(NULL),
	fSearchResults(NULL),
	fMenuBar(NULL),
	fFileMenu(NULL),
	fNew(NULL),
	fOpen(NULL),
	fClose(NULL),
	fQuit(NULL),
	fActionMenu(NULL),
	fSelectAll(NULL),
	fSearch(NULL),
	fTrimSelection(NULL),
	fCopyText(NULL),
	fSelectInTracker(NULL),
	fOpenSelection(NULL),
	fPreferencesMenu(NULL),
	fRecurseLinks(NULL),
	fRecurseDirs(NULL),
	fSkipDotDirs(NULL),
	fCaseSensitive(NULL),
	fRegularExpression(NULL),
	fTextOnly(NULL),
	fInvokeEditor(NULL),
	fHistoryMenu(NULL),
	fEncodingMenu(NULL),
	fUTF8(NULL),
	fShiftJIS(NULL),
	fEUC(NULL),
	fJIS(NULL),

	fShowLinesCheckbox(NULL),
	fButton(NULL),

	fGrepper(NULL),
	fOldPattern(""),
	fModel(new (nothrow) Model()),
	fLastNodeMonitorEvent(system_time()),
	fChangesIterator(NULL),
	fChangesPulse(NULL),

	fFilePanel(NULL)
{
	if (fModel == NULL)
		return;

	entry_ref directory;
	_InitRefsReceived(&directory, message);

	fModel->fDirectory = directory;
	fModel->fSelectedFiles = *message;

	_SetWindowTitle();
	_CreateMenus();
	_CreateViews();
	_LayoutViews();
	_LoadPrefs();
	_TileIfMultipleWindows();

	Show();
}


GrepWindow::~GrepWindow()
{
	delete fGrepper;
	delete fModel;
}


void GrepWindow::FrameResized(float width, float height)
{
	BWindow::FrameResized(width, height);
	fModel->fFrame = Frame();
	_SavePrefs();
}


void GrepWindow::FrameMoved(BPoint origin)
{
	BWindow::FrameMoved(origin);
	fModel->fFrame = Frame();
	_SavePrefs();
}


void GrepWindow::MenusBeginning()
{
	fModel->FillHistoryMenu(fHistoryMenu);
	BWindow::MenusBeginning();
}


void GrepWindow::MenusEnded()
{
	for (int32 t = fHistoryMenu->CountItems(); t > 0; --t)
		delete fHistoryMenu->RemoveItem(t - 1);

	BWindow::MenusEnded();
}


void GrepWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_NEW_WINDOW:
			_OnNewWindow();
			break;

		case B_SIMPLE_DATA:
			_OnFileDrop(message);
			break;

		case MSG_OPEN_PANEL:
			_OnOpenPanel();
			break;

		case MSG_REFS_RECEIVED:
			_OnRefsReceived(message);
			break;

		case B_CANCEL:
			_OnOpenPanelCancel();
			break;

		case MSG_RECURSE_LINKS:
			_OnRecurseLinks();
			break;

		case MSG_RECURSE_DIRS:
			_OnRecurseDirs();
			break;

		case MSG_SKIP_DOT_DIRS:
			_OnSkipDotDirs();
			break;

		case MSG_CASE_SENSITIVE:
			_OnCaseSensitive();
			break;

		case MSG_REGULAR_EXPRESSION:
			_OnRegularExpression();
			break;

		case MSG_TEXT_ONLY:
			_OnTextOnly();
			break;

		case MSG_INVOKE_EDITOR:
			_OnInvokeEditor();
			break;

		case MSG_SEARCH_TEXT:
			_OnSearchText();
			break;

		case MSG_SELECT_HISTORY:
			_OnHistoryItem(message);
			break;

		case MSG_START_CANCEL:
			_OnStartCancel();
			break;

		case MSG_SEARCH_FINISHED:
			_OnSearchFinished();
			break;

		case MSG_START_NODE_MONITORING:
			_StartNodeMonitoring();
			break;

		case B_PATH_MONITOR:
			_OnNodeMonitorEvent(message);
			break;

		case MSG_NODE_MONITOR_PULSE:
			_OnNodeMonitorPulse();
			break;

		case MSG_REPORT_FILE_NAME:
			_OnReportFileName(message);
			break;

		case MSG_REPORT_RESULT:
			_OnReportResult(message);
			break;

		case MSG_REPORT_ERROR:
			_OnReportError(message);
			break;

		case MSG_SELECT_ALL:
			_OnSelectAll(message);
			break;

		case MSG_TRIM_SELECTION:
			_OnTrimSelection();
			break;

		case MSG_COPY_TEXT:
			_OnCopyText();
			break;

		case MSG_SELECT_IN_TRACKER:
			_OnSelectInTracker();
			break;

		case MSG_CHECKBOX_SHOW_LINES:
			_OnCheckboxShowLines();
			break;

		case MSG_OPEN_SELECTION:
			// fall through
		case MSG_INVOKE_ITEM:
			_OnInvokeItem();
			break;

		case MSG_QUIT_NOW:
			_OnQuitNow();
			break;

		case 'utf8':
			fModel->fEncoding = 0;
			break;

		case B_SJIS_CONVERSION:
			fModel->fEncoding = B_SJIS_CONVERSION;
			break;

		case B_EUC_CONVERSION:
			fModel->fEncoding = B_EUC_CONVERSION;
			break;

		case B_JIS_CONVERSION:
			fModel->fEncoding = B_JIS_CONVERSION;
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
GrepWindow::Quit()
{
	CALLED();

	_StopNodeMonitoring();
	_SavePrefs();

	// TODO: stippi: Looks like this could be done
	// by maintaining a counter in GrepApp with the number of open
	// grep windows... and just quit when it goes zero
	if (be_app->Lock()) {
		be_app->PostMessage(MSG_TRY_QUIT);
		be_app->Unlock();
		BWindow::Quit();
	}
}


// #pragma mark -


void
GrepWindow::_InitRefsReceived(entry_ref* directory, BMessage* message)
{
	// HACK-HACK-HACK:
	// If the user selected a single folder and invoked TextSearch on it,
	// but recurse directories is switched off, TextSearch would do nothing.
	// In that special case, we'd like it to recurse into that folder (but
	// not go any deeper after that).

	type_code code;
	int32 count;
	message->GetInfo("refs", &code, &count);

	if (count == 0) {
		if (message->FindRef("dir_ref", 0, directory) == B_OK)
			message->MakeEmpty();
	}

	if (count == 1) {
		entry_ref ref;
		if (message->FindRef("refs", 0, &ref) == B_OK) {
			BEntry entry(&ref, true);
			if (entry.IsDirectory()) {
				// ok, special case, we use this folder as base directory
				// and pretend nothing had been selected:
				*directory = ref;
				message->MakeEmpty();
			}
		}
	}
}


void
GrepWindow::_SetWindowTitle()
{
	BEntry entry(&fModel->fDirectory, true);
	BString title;
	if (entry.InitCheck() == B_OK) {
		BPath path;
		if (entry.GetPath(&path) == B_OK) {
			if (fOldPattern.Length()) {
				title = B_TRANSLATE("%appname% : %path% : %searchtext%");
				title.ReplaceAll("%searchtext%", fOldPattern.String());
			} else
				title = B_TRANSLATE("%appname% : %path%");

			title.ReplaceAll("%appname%", B_TRANSLATE(APP_NAME));
			title.ReplaceAll("%path%", path.Path());
		}
	}

	if (!title.Length())
		title = B_TRANSLATE(APP_NAME);

	SetTitle(title.String());
}


void
GrepWindow::_CreateMenus()
{
	fMenuBar = new BMenuBar("menubar");

	fFileMenu = new BMenu(B_TRANSLATE("File"));
	fActionMenu = new BMenu(B_TRANSLATE("Actions"));
	fPreferencesMenu = new BMenu(B_TRANSLATE("Settings"));
	fHistoryMenu = new BMenu(B_TRANSLATE("History"));
	fEncodingMenu = new BMenu(B_TRANSLATE("Encoding"));

	fNew = new BMenuItem(
		B_TRANSLATE("New window"), new BMessage(MSG_NEW_WINDOW), 'N');

	fOpen = new BMenuItem(
		B_TRANSLATE("Set target" B_UTF8_ELLIPSIS), new BMessage(MSG_OPEN_PANEL), 'F');

	fClose = new BMenuItem(
		B_TRANSLATE("Close"), new BMessage(B_QUIT_REQUESTED), 'W');

	fQuit = new BMenuItem(
		B_TRANSLATE("Quit"), new BMessage(MSG_QUIT_NOW), 'Q');

	fSearch = new BMenuItem(
		B_TRANSLATE("Search"), new BMessage(MSG_START_CANCEL), 'S');

	fSelectAll = new BMenuItem(
		B_TRANSLATE("Select all"), new BMessage(MSG_SELECT_ALL), 'A');

	fTrimSelection = new BMenuItem(
		B_TRANSLATE("Trim to selection"), new BMessage(MSG_TRIM_SELECTION), 'T');

	fOpenSelection = new BMenuItem(
		B_TRANSLATE("Open selection"), new BMessage(MSG_OPEN_SELECTION), 'O');

	fSelectInTracker = new BMenuItem(
		B_TRANSLATE("Show files in Tracker"),
			new BMessage(MSG_SELECT_IN_TRACKER), 'K');

	fCopyText = new BMenuItem(
		B_TRANSLATE("Copy text to clipboard"), new BMessage(MSG_COPY_TEXT), 'B');

	fRecurseLinks = new BMenuItem(
		B_TRANSLATE("Follow symbolic links"), new BMessage(MSG_RECURSE_LINKS));

	fRecurseDirs = new BMenuItem(
		B_TRANSLATE("Look in sub-folders"), new BMessage(MSG_RECURSE_DIRS));

	fSkipDotDirs = new BMenuItem(
		B_TRANSLATE("Skip folders starting with a dot"),
			new BMessage(MSG_SKIP_DOT_DIRS));

	fCaseSensitive = new BMenuItem(
		B_TRANSLATE("Case-sensitive"), new BMessage(MSG_CASE_SENSITIVE));

	fRegularExpression = new BMenuItem(
		B_TRANSLATE("Regular expression"), new BMessage(MSG_REGULAR_EXPRESSION));

	fTextOnly = new BMenuItem(
		B_TRANSLATE("Text files only"), new BMessage(MSG_TEXT_ONLY));

	fInvokeEditor = new BMenuItem(
		B_TRANSLATE("Open files in code editor"), new BMessage(MSG_INVOKE_EDITOR));

	fUTF8 = new BMenuItem("UTF8", new BMessage('utf8'));
	fShiftJIS = new BMenuItem("ShiftJIS", new BMessage(B_SJIS_CONVERSION));
	fEUC = new BMenuItem("EUC", new BMessage(B_EUC_CONVERSION));
	fJIS = new BMenuItem("JIS", new BMessage(B_JIS_CONVERSION));

	fFileMenu->AddItem(fNew);
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(fOpen);
	fFileMenu->AddItem(fClose);
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(fQuit);

	fActionMenu->AddItem(fSearch);
	fActionMenu->AddSeparatorItem();
	fActionMenu->AddItem(fSelectAll);
	fActionMenu->AddItem(fTrimSelection);
	fActionMenu->AddSeparatorItem();
	fActionMenu->AddItem(fOpenSelection);
	fActionMenu->AddItem(fSelectInTracker);
	fActionMenu->AddItem(fCopyText);

	fPreferencesMenu->AddItem(fRecurseLinks);
	fPreferencesMenu->AddItem(fRecurseDirs);
	fPreferencesMenu->AddItem(fSkipDotDirs);
	fPreferencesMenu->AddItem(fCaseSensitive);
	fPreferencesMenu->AddItem(fRegularExpression);
	fPreferencesMenu->AddItem(fTextOnly);
	fPreferencesMenu->AddItem(fInvokeEditor);

	fEncodingMenu->AddItem(fUTF8);
	fEncodingMenu->AddItem(fShiftJIS);
	fEncodingMenu->AddItem(fEUC);
	fEncodingMenu->AddItem(fJIS);

//	fEncodingMenu->SetLabelFromMarked(true);
		// Do we really want this ?
	fEncodingMenu->SetRadioMode(true);
	fEncodingMenu->ItemAt(0)->SetMarked(true);

	fMenuBar->AddItem(fFileMenu);
	fMenuBar->AddItem(fActionMenu);
	fMenuBar->AddItem(fPreferencesMenu);
	fMenuBar->AddItem(fHistoryMenu);
	fMenuBar->AddItem(fEncodingMenu);

	fSearch->SetEnabled(false);
}


void
GrepWindow::_CreateViews()
{
	// The search pattern entry field does not send a message when
	// <Enter> is pressed, because the "Search/Cancel" button already
	// does this and we don't want to send the same message twice.

	fSearchText = new BTextControl(
		"SearchText", NULL, NULL, NULL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE);

	fSearchText->TextView()->SetMaxBytes(1000);
	fSearchText->SetModificationMessage(new BMessage(MSG_SEARCH_TEXT));

	fButton = new BButton(
		"Button", B_TRANSLATE("Search"),
		new BMessage(MSG_START_CANCEL));
	fButton->MakeDefault(true);
	fButton->SetEnabled(false);

	fShowLinesCheckbox = new BCheckBox(
		"ShowLines", B_TRANSLATE("Show lines"),
		new BMessage(MSG_CHECKBOX_SHOW_LINES));
	fShowLinesCheckbox->SetValue(B_CONTROL_ON);

	fSearchResults = new GrepListView();
	fSearchResults->SetInvocationMessage(new BMessage(MSG_INVOKE_ITEM));
}


void
GrepWindow::_LayoutViews()
{
	BScrollView* scroller = new BScrollView(
		"ScrollSearchResults", fSearchResults,
		B_FULL_UPDATE_ON_RESIZE, true, true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0, 0, -1, -1)
		.Add(fMenuBar)
		.AddGrid(B_USE_HALF_ITEM_INSETS)
			.SetInsets(B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING,
				B_USE_WINDOW_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fSearchText, 0, 0, 3)
			.Add(fShowLinesCheckbox, 0, 1)
			.Add(BSpaceLayoutItem::CreateGlue(), 1, 1)
			.Add(fButton, 2, 1)
		.End()
		.AddGroup(B_VERTICAL, 0)
			.SetInsets(-2, 0, -1, -1)
			.Add(scroller)
		.End()
	.End();

	fSearchText->MakeFocus(true);

	SetKeyMenuBar(fMenuBar);
}


void
GrepWindow::_TileIfMultipleWindows()
{
	if (be_app->Lock()) {
		int32 windowCount = be_app->CountWindows();
		be_app->Unlock();

		if (windowCount > 1)
			MoveBy(20, 20);
	}

	BScreen screen(this);
	BRect screenFrame = screen.Frame();
	BRect windowFrame = Frame();

	if (windowFrame.left > screenFrame.right
		|| windowFrame.top > screenFrame.bottom
		|| windowFrame.right < screenFrame.left
		|| windowFrame.bottom < screenFrame.top)
		MoveTo(50, 50);
}


// #pragma mark -


void
GrepWindow::_LoadPrefs()
{
	Lock();

	fModel->LoadPrefs();

	fRecurseDirs->SetMarked(fModel->fRecurseDirs);
	fRecurseLinks->SetMarked(fModel->fRecurseLinks);
	fSkipDotDirs->SetMarked(fModel->fSkipDotDirs);
	fCaseSensitive->SetMarked(fModel->fCaseSensitive);
	fRegularExpression->SetMarked(fModel->fRegularExpression);
	fTextOnly->SetMarked(fModel->fTextOnly);
	fInvokeEditor->SetMarked(fModel->fInvokeEditor);

	fShowLinesCheckbox->SetValue(fModel->fShowLines);

	switch (fModel->fEncoding) {
		case 0:
			fUTF8->SetMarked(true);
			break;
		case B_SJIS_CONVERSION:
			fShiftJIS->SetMarked(true);
			break;
		case B_EUC_CONVERSION:
			fEUC->SetMarked(true);
			break;
		case B_JIS_CONVERSION:
			fJIS->SetMarked(true);
			break;
		default:
			printf("Woops. Bad fModel->fEncoding value.\n");
			break;
	}

	MoveTo(fModel->fFrame.left, fModel->fFrame.top);
	ResizeTo(fModel->fFrame.Width(), fModel->fFrame.Height());

	Unlock();
}


void
GrepWindow::_SavePrefs()
{
	fModel->SavePrefs();
}


void
GrepWindow::_StartNodeMonitoring()
{
	CALLED();

	_StopNodeMonitoring();

	BMessenger messenger(this);
	uint32 fileFlags = B_WATCH_NAME | B_WATCH_STAT | B_WATCH_ATTR;


	// watch the top level folder only, rest should be done through filtering
	// the node monitor notifications
	BPath path(&fModel->fDirectory);
	if (path.InitCheck() == B_OK) {
		TRACE_NM("start monitoring root folder: %s\n", path.Path());
		BPrivate::BPathMonitor::StartWatching(path.Path(),
			fileFlags | B_WATCH_RECURSIVELY | B_WATCH_FILES_ONLY, messenger);
	}

	if (fChangesPulse == NULL) {
		BMessage message(MSG_NODE_MONITOR_PULSE);
		fChangesPulse = new BMessageRunner(BMessenger(this), &message,
			kChangesPulseInterval);
	}
}


void
GrepWindow::_StopNodeMonitoring()
{
	if (fChangesPulse == NULL)
		return;

	CALLED();

	BPrivate::BPathMonitor::StopWatching(BMessenger(this));
	delete fChangesIterator;
	fChangesIterator = NULL;
	delete fChangesPulse;
	fChangesPulse = NULL;
}


// #pragma mark - events


void
GrepWindow::_OnStartCancel()
{
	CALLED();

	_StopNodeMonitoring();

	if (fModel->fState == STATE_IDLE) {
		fSearchResults->MakeEmpty();

		if (fSearchText->TextView()->TextLength() == 0)
			return;

		fModel->fState = STATE_SEARCH;

		fModel->AddToHistory(fSearchText->Text());

		// From now on, we don't want to be notified when the
		// search pattern changes, because the control will be
		// displaying the names of the files we are grepping.

		fSearchText->SetModificationMessage(NULL);

		fFileMenu->SetEnabled(false);
		fActionMenu->SetEnabled(false);
		fPreferencesMenu->SetEnabled(false);
		fHistoryMenu->SetEnabled(false);
		fEncodingMenu->SetEnabled(false);

		fSearchText->SetEnabled(false);

		fButton->MakeFocus(true);
		fButton->SetLabel(B_TRANSLATE("Cancel"));

		fSearch->SetEnabled(false);

		// We need to remember the search pattern, because during
		// the grepping, the text control's text will be replaced
		// by the name of the file that's currently being grepped.
		// When the grepping finishes, we need to restore the old
		// search pattern.

		fOldPattern = fSearchText->Text();

		_SetWindowTitle();

		FileIterator* iterator = new (nothrow) InitialIterator(fModel);
		fGrepper = new (nothrow) Grepper(fOldPattern.String(), fModel,
			this, iterator);
		if (fGrepper != NULL && fGrepper->IsValid())
			fGrepper->Start();
		else {
			// roll back in case of problems
			if (fGrepper == NULL)
				delete iterator;
			else {
				// Grepper owns iterator
				delete fGrepper;
				fGrepper = NULL;
			}
			fModel->fState = STATE_IDLE;
			// TODO: better notification to user
			fprintf(stderr, "Out of memory.\n");
		}
	} else if (fModel->fState == STATE_SEARCH) {
		fModel->fState = STATE_CANCEL;
		fGrepper->Cancel();
	}
}


void
GrepWindow::_OnSearchFinished()
{
	fModel->fState = STATE_IDLE;

	delete fGrepper;
	fGrepper = NULL;

	fFileMenu->SetEnabled(true);
	fActionMenu->SetEnabled(true);
	fPreferencesMenu->SetEnabled(true);
	fHistoryMenu->SetEnabled(true);
	fEncodingMenu->SetEnabled(true);

	fButton->SetLabel(B_TRANSLATE("Search"));

	fButton->SetEnabled(true);
	fSearch->SetEnabled(true);

	fSearchText->SetEnabled(true);
	fSearchText->MakeFocus(true);
	fSearchText->SetText(fOldPattern.String());
	fSearchText->TextView()->SelectAll();
	fSearchText->SetModificationMessage(new BMessage(MSG_SEARCH_TEXT));

	PostMessage(MSG_START_NODE_MONITORING);
}


void
GrepWindow::_OnNodeMonitorEvent(BMessage* message)
{
	int32 opCode;
	if (message->FindInt32("opcode", &opCode) != B_OK)
		return;

	if (fChangesIterator == NULL) {
		fChangesIterator = new (nothrow) ChangesIterator(fModel);
		if (fChangesIterator == NULL || !fChangesIterator->IsValid()) {
			delete fChangesIterator;
			fChangesIterator = NULL;
		}
	}

	switch (opCode) {
		case B_ENTRY_CREATED:
		case B_ENTRY_REMOVED:
		{
			TRACE_NM("%s\n", opCode == B_ENTRY_CREATED ? "B_ENTRY_CREATED"
				: "B_ENTRY_REMOVED");
			BString path;
			if (message->FindString("path", &path) == B_OK) {
				if (opCode == B_ENTRY_CREATED) {
					if (fChangesIterator != NULL)
						fChangesIterator->EntryAdded(path.String());
				} else {
					// in order to remove temporary files
					if (fChangesIterator != NULL)
						fChangesIterator->EntryRemoved(path.String());
					// remove from the list view already
					BEntry entry(path.String());
					entry_ref ref;
					if (entry.GetRef(&ref) == B_OK)
						fSearchResults->RemoveResults(ref, true);
				}
			} else {
				#ifdef TRACE_NODE_MONITORING
					printf("incompatible message:\n");
					message->PrintToStream();
				#endif
			}
			TRACE_NM("path: %s\n", path.String());
			break;
		}
		case B_ENTRY_MOVED:
		{
			TRACE_NM("B_ENTRY_MOVED\n");

			BString path;
			if (message->FindString("path", &path) != B_OK) {
				#ifdef TRACE_NODE_MONITORING
					printf("incompatible message:\n");
					message->PrintToStream();
				#endif
				break;
			}

			bool added;
			if (message->FindBool("added", &added) != B_OK)
				added = false;
			bool removed;
			if (message->FindBool("removed", &removed) != B_OK)
				removed = false;

			if (added) {
				// new files
			} else if (removed) {
				// remove files
			} else {
				// files changed location, but are still within the search
				// path!
				BEntry entry(path.String());
				entry_ref ref;
				if (entry.GetRef(&ref) == B_OK) {
					int32 index;
					ResultItem* item = fSearchResults->FindItem(ref, &index);
					if (item != NULL) {
						item->SetText(path.String());
						// take care of invalidation, the index is currently
						// the full list index, but needs to be the visible
						// items index for this
						index = fSearchResults->IndexOf(item);
						fSearchResults->InvalidateItem(index);
					}
				}
			}
			break;
		}
		case B_STAT_CHANGED:
		case B_ATTR_CHANGED:
		{
			TRACE_NM("%s\n", opCode == B_STAT_CHANGED ? "B_STAT_CHANGED"
				: "B_ATTR_CHANGED");
			// For directly watched files, the path will include the
			// name. When the event occurs for a file in a watched directory,
			// the message will have an extra name field for the respective
			// file.
			BString path;
			if (message->FindString("path", &path) == B_OK) {
				if (fChangesIterator != NULL)
					fChangesIterator->EntryChanged(path.String());
			} else {
				#ifdef TRACE_NODE_MONITORING
					printf("incompatible message:\n");
					message->PrintToStream();
				#endif
			}
			TRACE_NM("path: %s\n", path.String());
// message->PrintToStream();
			break;
		}

		default:
			TRACE_NM("unkown op code\n");
			break;
	}

	fLastNodeMonitorEvent = system_time();
}


void
GrepWindow::_OnNodeMonitorPulse()
{
	if (fChangesIterator == NULL || fChangesIterator->IsEmpty())
		return;

	if (system_time() - fLastNodeMonitorEvent < kChangesPulseInterval) {
		// wait for things to settle down before running the search for changes
		return;
	}

	if (fModel->fState != STATE_IDLE) {
		// An update or search is still in progress. New node monitor messages
		// may arrive while an update is still running. They should not arrive
		// during a regular search, but we want to be prepared for that anyways
		// and check != STATE_IDLE.
		return;
	}

	fOldPattern = fSearchText->Text();

#ifdef TRACE_NODE_MONITORING
	fChangesIterator->PrintToStream();
#endif

	fGrepper = new (nothrow) Grepper(fOldPattern.String(), fModel,
		this, fChangesIterator);
	if (fGrepper != NULL && fGrepper->IsValid()) {
		fGrepper->Start();
		fChangesIterator = NULL;
		fModel->fState = STATE_UPDATE;
	} else {
		// roll back in case of problems
		if (fGrepper == NULL)
			delete fChangesIterator;
		else {
			// Grepper owns iterator
			delete fGrepper;
			fGrepper = NULL;
		}
		fprintf(stderr, "Out of memory.\n");
	}
}


void
GrepWindow::_OnReportFileName(BMessage* message)
{
	if (fModel->fState != STATE_UPDATE) {
		BString name = message->FindString("filename");
		fSearchText->TruncateString(&name, B_TRUNCATE_MIDDLE,
			fSearchText->Bounds().Width() - 10);

		fSearchText->SetText(name);
	}
}


void
GrepWindow::_OnReportResult(BMessage* message)
{
	CALLED();
	entry_ref ref;
	if (message->FindRef("ref", &ref) != B_OK)
		return;

	type_code type;
	int32 count;
	message->GetInfo("text", &type, &count);

	BStringItem* item = NULL;
	if (fModel->fState == STATE_UPDATE) {
		// During updates because of node monitor events, negatives are
		// also reported (count == 0).
		item = fSearchResults->RemoveResults(ref, count == 0);
	}

	if (count == 0)
		return;

	if (item == NULL) {
		item = new ResultItem(ref);
		fSearchResults->AddItem(item);
		item->SetExpanded(fShowLinesCheckbox->Value() == 1);
	}

	const char* buf;
	while (message->FindString("text", --count, &buf) == B_OK) {
		uchar* temp = (uchar*)strdup(buf);
		uchar* ptr = temp;

		while (true) {
			// replace all non-printable characters by spaces
			uchar c = *ptr;

			if (c == '\0')
				break;

			if (!(c & 0x80) && iscntrl(c))
				*ptr = ' ';

			++ptr;
		}

		fSearchResults->AddUnder(new BStringItem((const char*)temp), item);

		free(temp);
	}
}


void
GrepWindow::_OnReportError(BMessage* message)
{
	const char* buf;
	if (message->FindString("error", &buf) == B_OK)
		fSearchResults->AddItem(new BStringItem(buf));
}


void
GrepWindow::_OnRecurseLinks()
{
	fModel->fRecurseLinks = !fModel->fRecurseLinks;
	fRecurseLinks->SetMarked(fModel->fRecurseLinks);
	_ModelChanged();
}


void
GrepWindow::_OnRecurseDirs()
{
	fModel->fRecurseDirs = !fModel->fRecurseDirs;
	fRecurseDirs->SetMarked(fModel->fRecurseDirs);
	_ModelChanged();
}


void
GrepWindow::_OnSkipDotDirs()
{
	fModel->fSkipDotDirs = !fModel->fSkipDotDirs;
	fSkipDotDirs->SetMarked(fModel->fSkipDotDirs);
	_ModelChanged();
}


void
GrepWindow::_OnRegularExpression()
{
	fModel->fRegularExpression = !fModel->fRegularExpression;
	fRegularExpression->SetMarked(fModel->fRegularExpression);
	_ModelChanged();
}


void
GrepWindow::_OnCaseSensitive()
{
	fModel->fCaseSensitive = !fModel->fCaseSensitive;
	fCaseSensitive->SetMarked(fModel->fCaseSensitive);
	_ModelChanged();
}


void
GrepWindow::_OnTextOnly()
{
	fModel->fTextOnly = !fModel->fTextOnly;
	fTextOnly->SetMarked(fModel->fTextOnly);
	_ModelChanged();
}


void
GrepWindow::_OnInvokeEditor()
{
	fModel->fInvokeEditor = !fModel->fInvokeEditor;
	fInvokeEditor->SetMarked(fModel->fInvokeEditor);
	_SavePrefs();
}


void
GrepWindow::_OnCheckboxShowLines()
{
	// Selection in BOutlineListView in multiple selection mode
	// gets weird when collapsing. I've tried all sorts of things.
	// It seems impossible to make it behave just right.

	// Going from collapsed to expande mode, the superitems
	// keep their selection, the subitems don't (yet) have
	// a selection. This works as expected, AFAIK.

	// Going from expanded to collapsed mode, I would like
	// for a selected subitem (line) to select its superitem,
	// (its file) and the subitem be unselected.

	// I've successfully tried code patches that apply the
	// selection pattern that I want, but with weird effects
	// on subsequent manual selection.
	// Lines stay selected while the user tries to select
	// some other line. It just gets weird.

	// It's as though listItem->Select() and Deselect()
	// put the items in some semi-selected state.
	// Or maybe I've got it all wrong.

	// So, here's the plain basic collapse/expand.
	// I think it's the least bad of what's possible on BeOS R5,
	// but perhaps someone comes along with a patch of magic.

	fModel->fShowLines = (fShowLinesCheckbox->Value() == 1);

	int32 numItems = fSearchResults->FullListCountItems();
	for (int32 x = 0; x < numItems; ++x) {
		BListItem* listItem = fSearchResults->FullListItemAt(x);
		if (listItem->OutlineLevel() == 0) {
			if (fModel->fShowLines) {
				if (!fSearchResults->IsExpanded(x))
					fSearchResults->Expand(listItem);
			} else {
				if (fSearchResults->IsExpanded(x))
					fSearchResults->Collapse(listItem);
			}
		}
	}

	fSearchResults->Invalidate();

	_SavePrefs();
}


void
GrepWindow::_OnInvokeItem()
{
	for (int32 selectionIndex = 0; ; selectionIndex++) {
		int32 itemIndex = fSearchResults->CurrentSelection(selectionIndex);
		BListItem* item = fSearchResults->ItemAt(itemIndex);
		if (item == NULL)
			break;

		int32 level = item->OutlineLevel();
		int32 lineNum = -1;

		// Get the line number.
		// only this level has line numbers
		if (level == 1) {
			BStringItem* str = dynamic_cast<BStringItem*>(item);
			if (str != NULL) {
				lineNum = atol(str->Text());
					// fortunately, atol knows when to stop the conversion
			}
		}

		// Get the top-most item and launch its entry_ref.
		while (level != 0) {
			item = fSearchResults->Superitem(item);
			if (item == NULL)
				break;
			level = item->OutlineLevel();
		}

		ResultItem* entry = dynamic_cast<ResultItem*>(item);
		if (entry != NULL) {
			if (fModel->fInvokeEditor && _OpenInEditor(entry->ref, lineNum))
				return;

			// ask tracker to open it for us
			BMessenger target(TRACKER_SIGNATURE);
			BMessage message(B_REFS_RECEIVED);
			message.AddRef("refs", &entry->ref);
			if (lineNum > -1) {
				message.AddInt32("be:line", lineNum);
			}
			target.SendMessage(&message);
		}
	}
}


void
GrepWindow::_OnSearchText()
{
	CALLED();

	bool enabled = fSearchText->TextView()->TextLength() != 0;
	fButton->SetEnabled(enabled);
	fSearch->SetEnabled(enabled);
	_StopNodeMonitoring();
}


void
GrepWindow::_OnHistoryItem(BMessage* message)
{
	const char* buf;
	if (message->FindString("text", &buf) == B_OK)
		fSearchText->SetText(buf);
}


void
GrepWindow::_OnTrimSelection()
{
	if (fSearchResults->CurrentSelection() < 0) {
		BString text;
		text << B_TRANSLATE("Please select the files you wish to keep searching.");
		text << "\n";
		text << B_TRANSLATE("The unselected files will be removed from the list.");
		text << "\n";
		BAlert* alert = new BAlert(NULL, text.String(), B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
		return;
	}

	BMessage message;
	BString path;

	for (int32 index = 0; ; index++) {
		BStringItem* item = dynamic_cast<BStringItem*>(
			fSearchResults->ItemAt(index));
		if (item == NULL)
			break;

		if (!item->IsSelected() || item->OutlineLevel() != 0)
			continue;

		if (path == item->Text())
			continue;

		path = item->Text();
		entry_ref ref;
		if (get_ref_for_path(path.String(), &ref) == B_OK)
			message.AddRef("refs", &ref);
	}

	fModel->fDirectory = entry_ref();
		// invalidated on purpose

	fModel->fSelectedFiles.MakeEmpty();
	fModel->fSelectedFiles = message;

	PostMessage(MSG_START_CANCEL);

	_SetWindowTitle();
}


void
GrepWindow::_OnCopyText()
{
	bool onlyCopySelection = true;

	if (fSearchResults->CurrentSelection() < 0)
		onlyCopySelection = false;

	BString buffer;

	for (int32 index = 0; ; index++) {
		BStringItem* item = dynamic_cast<BStringItem*>(
			fSearchResults->ItemAt(index));
		if (item == NULL)
			break;

		if (onlyCopySelection) {
			if (item->IsSelected())
				buffer << item->Text() << "\n";
		} else
			buffer << item->Text() << "\n";
	}

	status_t status = B_OK;

	BMessage* clip = NULL;

	if (be_clipboard->Lock()) {
		be_clipboard->Clear();

		clip = be_clipboard->Data();

		clip->AddData("text/plain", B_MIME_TYPE, buffer.String(),
			buffer.Length());

		status = be_clipboard->Commit();

		if (status != B_OK) {
			be_clipboard->Unlock();
			return;
		}

		be_clipboard->Unlock();
	}
}


void
GrepWindow::_OnSelectInTracker()
{
	if (fSearchResults->CurrentSelection() < 0) {
		BAlert* alert = new BAlert("Info",
			B_TRANSLATE("Please select the files you wish to have selected for you in "
				"Tracker."),
			B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
		return;
	}

	BMessage message;
	BString filePath;
	BPath folderPath;
	BList folderList;
	BString lastFolderAddedToList;

	for (int32 index = 0; ; index++) {
		BStringItem* item = dynamic_cast<BStringItem*>(
			fSearchResults->ItemAt(index));
		if (item == NULL)
			break;

		// only open selected and top level (file) items
		if (!item->IsSelected() || item->OutlineLevel() > 0)
			continue;

		// check if this was previously opened
		if (filePath == item->Text())
			continue;

		filePath = item->Text();
		entry_ref file_ref;
		if (get_ref_for_path(filePath.String(), &file_ref) != B_OK)
			continue;

		message.AddRef("refs", &file_ref);

		// add parent folder to list of folders to open
		folderPath.SetTo(filePath.String());
		if (folderPath.GetParent(&folderPath) == B_OK) {
			BPath* path = new BPath(folderPath);
			if (path->Path() != lastFolderAddedToList) {
				// catches some duplicates
				folderList.AddItem(path);
				lastFolderAddedToList = path->Path();
			} else
				delete path;
		}
	}

	_RemoveFolderListDuplicates(&folderList);
	_OpenFoldersInTracker(&folderList);

	int32 aShortWhile = 100000;
	snooze(aShortWhile);

	if (!_AreAllFoldersOpenInTracker(&folderList)) {
		for (int32 x = 0; x < 5; x++) {
			aShortWhile += 100000;
			snooze(aShortWhile);
			_OpenFoldersInTracker(&folderList);
		}
	}

	if (!_AreAllFoldersOpenInTracker(&folderList)) {
		BString str1;
		str1 << B_TRANSLATE("%APP_NAME couldn't open one or more folders.");
		str1.ReplaceFirst("%APP_NAME",APP_NAME);
		BAlert* alert = new BAlert(NULL, str1.String(), B_TRANSLATE("OK"),
			NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
		goto out;
	}

	_SelectFilesInTracker(&folderList, &message);

out:
	// delete folderList contents
	int32 folderCount = folderList.CountItems();
	for (int32 x = 0; x < folderCount; x++)
		delete static_cast<BPath*>(folderList.ItemAt(x));
}


void
GrepWindow::_OnQuitNow()
{
	if (be_app->Lock()) {
		be_app->PostMessage(B_QUIT_REQUESTED);
		be_app->Unlock();
	}
}


void
GrepWindow::_OnFileDrop(BMessage* message)
{
	if (fModel->fState != STATE_IDLE)
		return;

	entry_ref directory;
	_InitRefsReceived(&directory, message);

	fModel->fDirectory = directory;
	fModel->fSelectedFiles.MakeEmpty();
	fModel->fSelectedFiles = *message;

	fSearchResults->MakeEmpty();
	fOldPattern = "";

	_SetWindowTitle();
}


void
GrepWindow::_OnRefsReceived(BMessage* message)
{
	_OnFileDrop(message);
	fOldPattern = "";
	// It seems a B_CANCEL always follows a B_REFS_RECEIVED
	// from a BFilePanel in Open mode.
	//
	// _OnOpenPanelCancel() is called on B_CANCEL.
	// That's where saving the current dir of the file panel occurs, for now,
	// and also the neccesary deletion of the file panel object.
	// A hidden file panel would otherwise jam the shutdown process.
}


void
GrepWindow::_OnOpenPanel()
{
	if (fFilePanel != NULL)
		return;

	entry_ref path;
	if (get_ref_for_path(fModel->fFilePanelPath.String(), &path) != B_OK)
		return;

	BMessenger messenger(this);
	BMessage message(MSG_REFS_RECEIVED);
	fFilePanel = new BFilePanel(B_OPEN_PANEL, &messenger, &path,
		B_FILE_NODE | B_DIRECTORY_NODE | B_SYMLINK_NODE, true,
		&message, NULL, true, true);

	fFilePanel->Show();
}


void
GrepWindow::_OnOpenPanelCancel()
{
	entry_ref panelDirRef;
	fFilePanel->GetPanelDirectory(&panelDirRef);
	BPath path(&panelDirRef);
	fModel->fFilePanelPath = path.Path();
	delete fFilePanel;
	fFilePanel = NULL;
}


void
GrepWindow::_OnSelectAll(BMessage* message)
{
	BMessenger messenger(fSearchResults);
	messenger.SendMessage(B_SELECT_ALL);
}


void
GrepWindow::_OnNewWindow()
{
	BMessage cloneRefs;
		// we don't want GrepWindow::InitRefsReceived()
		// to mess with the refs of the current window

	cloneRefs = fModel->fSelectedFiles;
	cloneRefs.AddRef("dir_ref", &(fModel->fDirectory));

	new GrepWindow(&cloneRefs);
}


// #pragma mark -


void
GrepWindow::_ModelChanged()
{
	CALLED();

	_StopNodeMonitoring();
	_SavePrefs();
}

bool
GrepWindow::_OpenInEditor(const entry_ref &ref, int32 lineNum)
{
	BMessage message(B_REFS_RECEIVED);
	message.AddRef("refs", &ref);

	if (lineNum != -1) {
		message.AddInt32("line", lineNum);	// for Pe
		message.AddInt32("be:line", lineNum);
	}

	// Find the preferred code editor
	char editorSig[B_MIME_TYPE_LENGTH];
	BMimeType mimeType("text/x-source-code");
	mimeType.GetPreferredApp(editorSig);

	entry_ref editor;
	if (be_roster->FindApp(editorSig, &editor) != B_OK)
		return false;

	if (be_roster->IsRunning(&editor)) {
		BMessenger msngr(NULL, be_roster->TeamFor(&editor));
		if (msngr.SendMessage(&message) != B_OK)
			return false;
	} else {
		if (be_roster->Launch(&editor, &message) != B_OK)
			return false;
	}

	return true;
}


void
GrepWindow::_RemoveFolderListDuplicates(BList* folderList)
{
	if (folderList == NULL)
		return;

	int32 folderCount = folderList->CountItems();
	BString folderX;
	BString folderY;

	for (int32 x = 0; x < folderCount; x++) {
		BPath* path = static_cast<BPath*>(folderList->ItemAt(x));
		folderX = path->Path();

		for (int32 y = x + 1; y < folderCount; y++) {
			path = static_cast<BPath*>(folderList->ItemAt(y));
			folderY = path->Path();
			if (folderX == folderY) {
				delete static_cast<BPath*>(folderList->RemoveItem(y));
				folderCount--;
				y--;
			}
		}
	}
}


status_t
GrepWindow::_OpenFoldersInTracker(BList* folderList)
{
	status_t status = B_OK;
	BMessage refsMsg(B_REFS_RECEIVED);

	int32 folderCount = folderList->CountItems();
	for (int32 index = 0; index < folderCount; index++) {
		BPath* path = static_cast<BPath*>(folderList->ItemAt(index));

		entry_ref folderRef;
		status = get_ref_for_path(path->Path(), &folderRef);
		if (status != B_OK)
			return status;

		status = refsMsg.AddRef("refs", &folderRef);
		if (status != B_OK)
			return status;
	}

	status = be_roster->Launch(TRACKER_SIGNATURE, &refsMsg);
	if (status != B_OK && status != B_ALREADY_RUNNING)
		return status;

	return B_OK;
}


bool
GrepWindow::_AreAllFoldersOpenInTracker(BList* folderList)
{
	// Compare the folders we want open in Tracker to
	// the actual Tracker windows currently open.

	// We build a list of open Tracker windows, and compare
	// it to the list of folders we want open in Tracker.

	// If all folders exists in the list of Tracker windows
	// return true

	status_t status = B_OK;
	BMessenger trackerMessenger(TRACKER_SIGNATURE);
	BMessage sendMessage;
	BMessage replyMessage;
	BList windowList;

	if (!trackerMessenger.IsValid())
		return false;

	for (int32 count = 1; ; count++) {
		sendMessage.MakeEmpty();
		replyMessage.MakeEmpty();

		sendMessage.what = B_GET_PROPERTY;
		sendMessage.AddSpecifier("Path");
		sendMessage.AddSpecifier("Poses");
		sendMessage.AddSpecifier("Window", count);

		status = trackerMessenger.SendMessage(&sendMessage, &replyMessage);
		if (status != B_OK)
			return false;

		entry_ref* trackerRef = new (nothrow) entry_ref;
		status = replyMessage.FindRef("result", trackerRef);
		if (status != B_OK || !windowList.AddItem(trackerRef)) {
			delete trackerRef;
			break;
		}
	}

	int32 folderCount = folderList->CountItems();
	int32 windowCount = windowList.CountItems();

	int32 found = 0;
	BPath* folderPath;
	entry_ref* windowRef;
	BString folderString;
	BString windowString;
	bool result = false;

	if (folderCount > windowCount) {
		// at least one folder is not open in Tracker
		goto out;
	}

	// Loop over the two lists and see if all folders exist as window
	for (int32 x = 0; x < folderCount; x++) {
		for (int32 y = 0; y < windowCount; y++) {

			folderPath = static_cast<BPath*>(folderList->ItemAt(x));
			windowRef = static_cast<entry_ref*>(windowList.ItemAt(y));

			if (folderPath == NULL)
				break;

			if (windowRef == NULL)
				break;

			folderString = folderPath->Path();

			BEntry entry;
			BPath path;

			if (entry.SetTo(windowRef) == B_OK && path.SetTo(&entry) == B_OK) {

				windowString = path.Path();

				if (folderString == windowString) {
					found++;
					break;
				}
			}
		}
	}

	result = found == folderCount;

out:
	// delete list of window entry_refs
	for (int32 x = 0; x < windowCount; x++)
		delete static_cast<entry_ref*>(windowList.ItemAt(x));

	return result;
}


status_t
GrepWindow::_SelectFilesInTracker(BList* folderList, BMessage* refsMessage)
{
	// loops over Tracker windows, find each windowRef,
	// extract the refs that are children of windowRef,
	// add refs to selection-message

	status_t status = B_OK;
	BMessenger trackerMessenger(TRACKER_SIGNATURE);
	BMessage windowSendMessage;
	BMessage windowReplyMessage;
	BMessage selectionSendMessage;
	BMessage selectionReplyMessage;

	if (!trackerMessenger.IsValid())
		return status;

	// loop over Tracker windows
	for (int32 windowCount = 1; ; windowCount++) {

		windowSendMessage.MakeEmpty();
		windowReplyMessage.MakeEmpty();

		windowSendMessage.what = B_GET_PROPERTY;
		windowSendMessage.AddSpecifier("Path");
		windowSendMessage.AddSpecifier("Poses");
		windowSendMessage.AddSpecifier("Window", windowCount);

		status = trackerMessenger.SendMessage(&windowSendMessage,
			&windowReplyMessage);

		if (status != B_OK)
			return status;

		entry_ref windowRef;
		status = windowReplyMessage.FindRef("result", &windowRef);
		if (status != B_OK)
			break;

		int32 folderCount = folderList->CountItems();

		// loop over folders in folderList
		for (int32 x = 0; x < folderCount; x++) {
			BPath* folderPath = static_cast<BPath*>(folderList->ItemAt(x));
			if (folderPath == NULL)
				break;

			BString folderString = folderPath->Path();

			BEntry windowEntry;
			BPath windowPath;
			BString windowString;

			status = windowEntry.SetTo(&windowRef);
			if (status != B_OK)
				break;

			status = windowPath.SetTo(&windowEntry);
			if (status != B_OK)
				break;

			windowString = windowPath.Path();

			// if match, loop over items in refsMessage
			// and add those that live in window/folder
			// to a selection message

			if (windowString == folderString) {
				selectionSendMessage.MakeEmpty();
				selectionSendMessage.what = B_SET_PROPERTY;
				selectionReplyMessage.MakeEmpty();

				// loop over refs and add to message
				entry_ref ref;
				for (int32 index = 0; ; index++) {
					status = refsMessage->FindRef("refs", index, &ref);
					if (status != B_OK)
						break;

					BDirectory directory(&windowRef);
					BEntry entry(&ref);
					if (directory.Contains(&entry))
						selectionSendMessage.AddRef("data", &ref);
				}

				// finish selection message
				selectionSendMessage.AddSpecifier("Selection");
				selectionSendMessage.AddSpecifier("Poses");
				selectionSendMessage.AddSpecifier("Window", windowCount);

				trackerMessenger.SendMessage(&selectionSendMessage,
					&selectionReplyMessage);
			}
		}
	}

	return B_OK;
}
