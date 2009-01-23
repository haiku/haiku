/*
 * Copyright 2007-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "SudokuWindow.h"

#include "CenteredViewContainer.h"
#include "ProgressWindow.h"
#include "Sudoku.h"
#include "SudokuField.h"
#include "SudokuGenerator.h"
#include "SudokuView.h"

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Roster.h>

#include <be_apps/Tracker/RecentItems.h>


const uint32 kMsgOpenFilePanel = 'opfp';
const uint32 kMsgGenerateVeryEasySudoku = 'gnsv';
const uint32 kMsgGenerateEasySudoku = 'gnse';
const uint32 kMsgGenerateHardSudoku = 'gnsh';
const uint32 kMsgAbortSudokuGenerator = 'asgn';
const uint32 kMsgSudokuGenerated = 'sugn';
const uint32 kMsgMarkInvalid = 'minv';
const uint32 kMsgMarkValidHints = 'mvht';
const uint32 kMsgStoreState = 'stst';
const uint32 kMsgRestoreState = 'rest';
const uint32 kMsgNew = 'new ';
const uint32 kMsgStartAgain = 'stag';
const uint32 kMsgExportAs = 'expt';


class GenerateSudoku {
public:
	GenerateSudoku(SudokuField& field, int32 level, BMessenger progress,
		BMessenger target);
	~GenerateSudoku();

	void Abort();

private:
	void _Generate();
	static status_t _GenerateThread(void* self);

	SudokuField	fField;
	BMessenger	fTarget;
	BMessenger	fProgress;
	thread_id	fThread;
	int32		fLevel;
	bool		fQuit;
};


GenerateSudoku::GenerateSudoku(SudokuField& field, int32 level,
		BMessenger progress, BMessenger target)
	:
	fField(field),
	fTarget(target),
	fProgress(progress),
	fLevel(level),
	fQuit(false)
{
	fThread = spawn_thread(_GenerateThread, "sudoku generator",
		B_LOW_PRIORITY, this);
	if (fThread >= B_OK)
		resume_thread(fThread);
	else
		_Generate();
}


GenerateSudoku::~GenerateSudoku()
{
	Abort();
}


void
GenerateSudoku::Abort()
{
	fQuit = true;

	status_t status;
	wait_for_thread(fThread, &status);
}


void
GenerateSudoku::_Generate()
{
	SudokuGenerator generator;

	bigtime_t start = system_time();
	generator.Generate(&fField, 40 - fLevel * 5, fProgress, &fQuit);
	printf("generated in %g msecs\n",
		(system_time() - start) / 1000.0);

	BMessage done(kMsgSudokuGenerated);
	if (!fQuit) {
		BMessage field;
		if (fField.Archive(&field, true) == B_OK)
			done.AddMessage("field", &field);
	}

	fTarget.SendMessage(&done);
}


/*static*/ status_t
GenerateSudoku::_GenerateThread(void* _self)
{
	GenerateSudoku* self = (GenerateSudoku*)_self;
	self->_Generate();
	return B_OK;
}


//	#pragma mark -


SudokuWindow::SudokuWindow()
	: BWindow(BRect(100, 100, 500, 520), "Sudoku", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE),
	fGenerator(NULL),
	fStoredState(NULL),
	fExportFormat(kExportAsText)
{
	BMessage settings;
	_LoadSettings(settings);

	BRect frame;
	if (settings.FindRect("window frame", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
		frame.OffsetTo(B_ORIGIN);
	} else
		frame = Bounds();

	if (settings.HasMessage("stored state")) {
		fStoredState = new BMessage;
		if (settings.FindMessage("stored state", fStoredState) != B_OK) {
			delete fStoredState;
			fStoredState = NULL;
		}
	}

	// create GUI

	BMenuBar* menuBar = new BMenuBar(Bounds(), "menu");
	AddChild(menuBar);

	frame.top = menuBar->Frame().bottom;

	BView* top = new BView(frame, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	top->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(top);

	fSudokuView = new SudokuView(top->Bounds().InsetByCopy(10, 10).OffsetToSelf(0, 0),
		"sudoku view", settings, B_FOLLOW_NONE);
	CenteredViewContainer * container = new CenteredViewContainer(fSudokuView, 
		top->Bounds().InsetByCopy(10, 10), 
		"center", B_FOLLOW_ALL);
	container->SetHighColor(top->ViewColor());
	top->AddChild(container);

	// add menu

	// "File" menu
	BMenu* menu = new BMenu("File");
	menu->AddItem(new BMenuItem("New", new BMessage(kMsgNew)));
	menu->AddItem(new BMenuItem("Start Again", new BMessage(kMsgStartAgain)));
	menu->AddSeparatorItem();
	BMenu* recentsMenu = BRecentFilesList::NewFileListMenu(
		"Open File" B_UTF8_ELLIPSIS, NULL, NULL, this, 10, false, NULL,
		kSignature);
	BMenuItem *item;
	menu->AddItem(item = new BMenuItem(recentsMenu,
		new BMessage(kMsgOpenFilePanel)));
	item->SetShortcut('O', B_COMMAND_KEY);
	BMenu* subMenu = new BMenu("Generate");
	subMenu->AddItem(new BMenuItem("Easy",
		new BMessage(kMsgGenerateVeryEasySudoku)));
	subMenu->AddItem(new BMenuItem("Advanced",
		new BMessage(kMsgGenerateEasySudoku)));
	subMenu->AddItem(new BMenuItem("Hard",
		new BMessage(kMsgGenerateHardSudoku)));
	menu->AddItem(subMenu);

	menu->AddSeparatorItem();

	subMenu = new BMenu("Export As" B_UTF8_ELLIPSIS);
	BMessage *msg;
	msg = new BMessage(kMsgExportAs);
	msg->AddInt32("as", kExportAsText);
	subMenu->AddItem(new BMenuItem("Text", msg));
	msg = new BMessage(kMsgExportAs);
	msg->AddInt32("as", kExportAsHTML);
	subMenu->AddItem(new BMenuItem("HTML", msg));
	/*
	msg = new BMessage(kMsgExportAs);
	msg->AddInt32("as", kExportAsBitmap);
	subMenu->AddItem(new BMenuItem("Bitmap" B_UTF8_ELLIPSIS, msg));
	*/
	menu->AddItem(subMenu);

	menu->AddItem(item = new BMenuItem("Copy",
		new BMessage(B_COPY), 'C'));

	menu->AddSeparatorItem();

	menu->AddItem(item = new BMenuItem("About Sudoku" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	menu->SetTargetForItems(this);
	item->SetTarget(be_app);
	menuBar->AddItem(menu);

	// "View" menu
	menu = new BMenu("View");
	menu->AddItem(item = new BMenuItem("Mark Invalid Values",
		new BMessage(kMsgMarkInvalid)));
	if (fSudokuView->HintFlags() & kMarkInvalid)
		item->SetMarked(true);
	menu->AddItem(item = new BMenuItem("Mark Valid Hints",
		new BMessage(kMsgMarkValidHints)));
	if (fSudokuView->HintFlags() & kMarkValidHints)
		item->SetMarked(true);
	menu->SetTargetForItems(this);
	menuBar->AddItem(menu);

	// "Help" menu
	menu = new BMenu("Help");
	menu->AddItem(fUndoItem = new BMenuItem("Undo", new BMessage(B_UNDO), 'Z'));
	fUndoItem->SetEnabled(false);
	menu->AddItem(fRedoItem = new BMenuItem("Redo", new BMessage(B_REDO), 'Z',
		B_SHIFT_KEY));
	fRedoItem->SetEnabled(false);
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Snapshot Current", new BMessage(kMsgStoreState)));
	menu->AddItem(fRestoreStateItem = new BMenuItem("Restore Snapshot",
		new BMessage(kMsgRestoreState)));
	fRestoreStateItem->SetEnabled(fStoredState != NULL);
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Solve", new BMessage(kMsgSolveSudoku)));
	menu->AddItem(new BMenuItem("Solve Single Field",
		new BMessage(kMsgSolveSingle)));
	menu->SetTargetForItems(fSudokuView);
	menuBar->AddItem(menu);

	fOpenPanel = new BFilePanel(B_OPEN_PANEL);
	fOpenPanel->SetTarget(this);
	fSavePanel = new BFilePanel(B_SAVE_PANEL);
	fSavePanel->SetTarget(this);

	fSudokuView->StartWatching(this, kUndoRedoChanged);
		// we like to know whenever the undo/redo state changes

	fProgressWindow = new ProgressWindow(this,
		new BMessage(kMsgAbortSudokuGenerator));
}


SudokuWindow::~SudokuWindow()
{
	delete fOpenPanel;
	delete fSavePanel;
	delete fGenerator;

	if (fProgressWindow->Lock())
		fProgressWindow->Quit();
}


status_t
SudokuWindow::_OpenSettings(BFile& file, uint32 mode)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("Sudoku settings");

	return file.SetTo(path.Path(), mode);
}


status_t
SudokuWindow::_LoadSettings(BMessage& settings)
{
	BFile file;
	status_t status = _OpenSettings(file, B_READ_ONLY);
	if (status < B_OK)
		return status;

	return settings.Unflatten(&file);
}


status_t
SudokuWindow::_SaveSettings()
{
	BFile file;
	status_t status = _OpenSettings(file, B_WRITE_ONLY | B_CREATE_FILE
		| B_ERASE_FILE);
	if (status < B_OK)
		return status;

	BMessage settings('sudo');
	status = settings.AddRect("window frame", Frame());
	if (status == B_OK)
		status = fSudokuView->SaveState(settings);
	if (status == B_OK && fStoredState != NULL)
		status = settings.AddMessage("stored state", fStoredState);
	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}


void
SudokuWindow::_ResetStoredState()
{
	delete fStoredState;
	fStoredState = NULL;
	fRestoreStateItem->SetEnabled(false);
}


void
SudokuWindow::_MessageDropped(BMessage* message)
{
	status_t status = B_MESSAGE_NOT_UNDERSTOOD;
	bool hasRef = false;

	entry_ref ref;
	if (message->FindRef("refs", &ref) != B_OK) {
		const void* data;
		ssize_t size;
		if (message->FindData("text/plain", B_MIME_TYPE, &data,
				&size) == B_OK) {
			status = fSudokuView->SetTo((const char*)data);
		} else
			return;
	} else {
		status = fSudokuView->SetTo(ref);
		if (status == B_OK)
			be_roster->AddToRecentDocuments(&ref, kSignature);

		BEntry entry(&ref);
		entry_ref parent;
		if (entry.GetParent(&entry) == B_OK
			&& entry.GetRef(&parent) == B_OK)
			fSavePanel->SetPanelDirectory(&parent);

		hasRef = true;
	}

	if (status < B_OK) {
		char buffer[1024];
		if (hasRef) {
			snprintf(buffer, sizeof(buffer),
				"Could not open \"%s\":\n"
				"%s", ref.name, strerror(status));
		} else {
			snprintf(buffer, sizeof(buffer), "Could not set Sudoku:\n%s",
				strerror(status));
		}

		(new BAlert("Sudoku request",
			buffer, "Ok", NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
	}
}


void
SudokuWindow::_Generate(int32 level)
{
	if (fGenerator != NULL)
		delete fGenerator;

	fSudokuView->SetEditable(false);
	fProgressWindow->Start(this);
	_ResetStoredState();

	fGenerator = new GenerateSudoku(*fSudokuView->Field(), level,
		fProgressWindow, this);
}


void
SudokuWindow::MessageReceived(BMessage* message)
{
	if (message->WasDropped()) {
		_MessageDropped(message);
		return;
	}

	switch (message->what) {
		case kMsgOpenFilePanel:
			fOpenPanel->Show();
			break;

		case B_REFS_RECEIVED:
		case B_SIMPLE_DATA:
			_MessageDropped(message);
			break;

		case kMsgGenerateVeryEasySudoku:
			_Generate(0);
			break;
		case kMsgGenerateEasySudoku:
			_Generate(2);
			break;
		case kMsgGenerateHardSudoku:
			_Generate(4);
			break;
		case kMsgAbortSudokuGenerator:
			if (fGenerator != NULL)
				fGenerator->Abort();
			break;
		case kMsgSudokuGenerated:
		{
			BMessage archive;
			if (message->FindMessage("field", &archive) == B_OK) {
				SudokuField* field = new SudokuField(&archive);
				fSudokuView->SetTo(field);
			}
			fSudokuView->SetEditable(true);
			fProgressWindow->Stop();

			delete fGenerator;
			fGenerator = NULL;
			break;
		}

		case kMsgExportAs:
		{
			if (message->FindInt32("as", (int32 *)&fExportFormat) < B_OK)
				fExportFormat = kExportAsText;
			fSavePanel->Show();
			break;
		}

		case B_COPY:
			fSudokuView->CopyToClipboard();
			break;

		case B_SAVE_REQUESTED:
		{
			entry_ref directoryRef;
			const char* name;
			if (message->FindRef("directory", &directoryRef) != B_OK
				|| message->FindString("name", &name) != B_OK)
				break;

			BDirectory directory(&directoryRef);
			BEntry entry(&directory, name);
			
			entry_ref ref;
			if (entry.GetRef(&ref) == B_OK)
				fSudokuView->SaveTo(ref, fExportFormat);
			break;
		}

		case kMsgNew:
			_ResetStoredState();
			fSudokuView->ClearAll();
			break;

		case kMsgStartAgain:
			fSudokuView->ClearChanged();
			break;

		case kMsgMarkInvalid:
		case kMsgMarkValidHints:
		{
			BMenuItem* item;
			if (message->FindPointer("source", (void**)&item) != B_OK)
				return;

			uint32 flag = message->what == kMsgMarkInvalid
				? kMarkInvalid : kMarkValidHints;

			item->SetMarked(!item->IsMarked());
			if (item->IsMarked())
				fSudokuView->SetHintFlags(fSudokuView->HintFlags() | flag);
			else
				fSudokuView->SetHintFlags(fSudokuView->HintFlags() & ~flag);
			break;
		}

		case kMsgStoreState:
			delete fStoredState;
			fStoredState = new BMessage;
			fSudokuView->Field()->Archive(fStoredState, true);
			fRestoreStateItem->SetEnabled(true);
			break;

		case kMsgRestoreState:
		{
			if (fStoredState == NULL)
				break;

			SudokuField* field = new SudokuField(fStoredState);
			fSudokuView->SetTo(field);
			break;
		}

		case kMsgSudokuSolved:
			(new BAlert("Sudoku request",
				"Sudoku solved - congratulations!", "Ok", NULL, NULL,
				B_WIDTH_AS_USUAL, B_IDEA_ALERT))->Go();
			break;

		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 what;
			if (message->FindInt32(B_OBSERVE_WHAT_CHANGE, &what) != B_OK)
				break;

			if (what == kUndoRedoChanged) {
				fUndoItem->SetEnabled(fSudokuView->CanUndo());
				fRedoItem->SetEnabled(fSudokuView->CanRedo());
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
SudokuWindow::QuitRequested()
{
	_SaveSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
