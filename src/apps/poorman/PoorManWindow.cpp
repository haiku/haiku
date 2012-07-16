/* PoorManWindow.cpp
 *
 *	Philip Harrison
 *	Started: 4/25/2004
 *	Version: 0.1
 */

#include "PoorManWindow.h"

#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include <Alert.h>
#include <Box.h>
#include <Catalog.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <OS.h>
#include <Path.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TypeConstants.h>

#include "PoorManApplication.h"
#include "PoorManPreferencesWindow.h"
#include "PoorManView.h"
#include "PoorManServer.h"
#include "PoorManLogger.h"
#include "constants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PoorMan"
#define DATE_FORMAT B_SHORT_DATE_FORMAT
#define TIME_FORMAT B_MEDIUM_TIME_FORMAT


PoorManWindow::PoorManWindow(BRect frame)
	:
	BWindow(frame, STR_APP_NAME, B_TITLED_WINDOW, 0),
	fStatus(false),
	fHits(0),
	fPrefWindow(NULL),
	fLogFile(NULL),
	fServer(NULL)
{
	//preferences init
	fWebDirectory.SetTo(STR_DEFAULT_WEB_DIRECTORY);
	fIndexFileName.SetTo("index.html");
	fDirListFlag = false;
	
	fLogConsoleFlag = true;
	fLogFileFlag = false;
	fLogPath.SetTo("");
	
	fMaxConnections = (int16)32;
	
	fIsZoomed = true;
	fLastWidth = 318.0f;
	fLastHeight = 320.0f;
	this->fFrame = frame;
	fSetwindowFrame.Set(112.0f, 60.0f, 492.0f, 340.0f);
	
	// PoorMan Window
	SetSizeLimits(318, 1600, 53, 1200); 
	// limit the size of the size of the window
	
	// -----------------------------------------------------------------
	// Three Labels 
	
	// Status String
	fStatusView = new BStringView("Status View", B_TRANSLATE("Status: Stopped"));
	
	// Directory String
	fDirView = new BStringView("Dir View", B_TRANSLATE("Directory: (none)"));
	
	// Hits String
	fHitsView = new BStringView("Hit View", B_TRANSLATE("Hits: 0"));

	// -----------------------------------------------------------------
	// Logging View

	fLoggingView = new BTextView(STR_TXT_VIEW, B_WILL_DRAW );

	fLoggingView->MakeEditable(false);	// user cannot change the text
	fLoggingView->MakeSelectable(true);
	fLoggingView->SetViewColor(WHITE);
	fLoggingView->SetStylable(true);
		
	// create the scroll view
	fScrollView = new BScrollView("Scroll View", fLoggingView,
					B_WILL_DRAW | B_FRAME_EVENTS | B_FOLLOW_ALL_SIDES, 
					// Make sure articles on border do not occur when resizing
					false, true);
	fLoggingView->MakeFocus(true);


	// -----------------------------------------------------------------
	// menu bar
	fFileMenuBar = new BMenuBar("File Menu Bar");
	
	// menus
	fFileMenu = BuildFileMenu();
	if (fFileMenu)
		fFileMenuBar->AddItem(fFileMenu);
			
	fEditMenu = BuildEditMenu();
	if (fEditMenu)
		fFileMenuBar->AddItem(fEditMenu);
		
	fControlsMenu = BuildControlsMenu();
	if (fControlsMenu)
		fFileMenuBar->AddItem(fControlsMenu);
	
	// File Panels
	BWindow* change_title;
	
	fSaveConsoleFilePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this),
						NULL, B_FILE_NODE, false,
						new BMessage(MSG_FILE_PANEL_SAVE_CONSOLE));
	change_title = fSaveConsoleFilePanel->Window();
	change_title->SetTitle(STR_FILEPANEL_SAVE_CONSOLE);
	
	fSaveConsoleSelectionFilePanel = new BFilePanel(B_SAVE_PANEL,
						new BMessenger(this), NULL, B_FILE_NODE, false,
						new BMessage(MSG_FILE_PANEL_SAVE_CONSOLE_SELECTION));
	change_title = fSaveConsoleSelectionFilePanel->Window();
	change_title->SetTitle(STR_FILEPANEL_SAVE_CONSOLE_SELECTION);
	
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0)
		.Add(fFileMenuBar)
		.AddGroup(B_VERTICAL, B_USE_SMALL_SPACING)
			.SetInsets(B_USE_WINDOW_INSETS)
			.AddGroup(B_HORIZONTAL)
				.Add(fStatusView)
				.AddGlue()
				.Add(fHitsView)
				.End()
			.AddGroup(B_HORIZONTAL)
				.Add(fDirView)
				.AddGlue()
				.End()
			.Add(fScrollView);
	
	pthread_rwlock_init(&fLogFileLock, NULL);
}


PoorManWindow::~PoorManWindow()
{
	delete fServer;
	delete fLogFile;
	pthread_rwlock_destroy(&fLogFileLock);
}


void
PoorManWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_MENU_FILE_SAVE_AS:
			fSaveConsoleFilePanel->Show();
			break;
		case MSG_FILE_PANEL_SAVE_CONSOLE:
			printf("FilePanel: Save console\n");
			SaveConsole(message, false);
			break;
		case MSG_MENU_FILE_SAVE_SELECTION:
			fSaveConsoleSelectionFilePanel->Show();
			break;
		case MSG_FILE_PANEL_SAVE_CONSOLE_SELECTION:
			printf("FilePanel: Save console selection\n");
			SaveConsole(message, true);
			break;
		case MSG_FILE_PANEL_SELECT_WEB_DIR:		
			fPrefWindow->MessageReceived(message);
			break;
		case MSG_MENU_EDIT_PREF:
			fPrefWindow = new PoorManPreferencesWindow(fSetwindowFrame,
				STR_WIN_NAME_PREF);
			fPrefWindow->Show();
			break;
		case MSG_MENU_CTRL_RUN:
			if (fStatus)
				StopServer();
			else
				StartServer();
			break;
		case MSG_MENU_CTRL_CLEAR_HIT:
			SetHits(0);
			//UpdateHitsLabel();
			break;
		case MSG_MENU_CTRL_CLEAR_CONSOLE:
			fLoggingView->SelectAll();
			fLoggingView->Delete();
			break;
		case MSG_MENU_CTRL_CLEAR_LOG:
			FILE* f;
			f = fopen(fLogPath.String(), "w");
			fclose(f);
			break;
		case MSG_LOG: {
			if (!fLogConsoleFlag && !fLogFileFlag)
				break;
	
			time_t time;
			in_addr_t address;
			rgb_color color;
			const void* pointer;
			ssize_t size;
			const char* msg;
			BString line;
		
			if (message->FindString("cstring", &msg) != B_OK)
				break;
			if (message->FindData("time_t", B_TIME_TYPE, &pointer, &size) != B_OK)
				time = -1;
			else
				time = *static_cast<const time_t*>(pointer);

			if (message->FindData("in_addr_t", B_ANY_TYPE, &pointer, &size) != B_OK)
				address = INADDR_NONE;
			else
				address = *static_cast<const in_addr_t*>(pointer);

			if (message->FindData("rgb_color", B_RGB_COLOR_TYPE, &pointer, &size) != B_OK)
				color = BLACK;
			else
				color = *static_cast<const rgb_color*>(pointer);
		
			if (time != -1) {
				BString timeString;
				if (BLocale::Default()->FormatDateTime(&timeString, time, 
						DATE_FORMAT, TIME_FORMAT) == B_OK) {
					line << '[' << timeString << "]: ";
				}
			}
		
			if (address != INADDR_NONE) {
				char addr[INET_ADDRSTRLEN];
				struct in_addr sin_addr;
				sin_addr.s_addr = address;
				if (inet_ntop(AF_INET, &sin_addr, addr, sizeof(addr)) != NULL) {
					addr[strlen(addr)] = '\0';
					line << '(' << addr << ") ";
				}
			}
		
			line << msg;
		
			text_run run;
			text_run_array runs;
		
			run.offset = 0;
			run.color = color;
		
			runs.count = 1;
			runs.runs[0] = run;
		
			if (Lock()) {
				if (fLogConsoleFlag) {
					fLoggingView->Insert(fLoggingView->TextLength(),
						line.String(), line.Length(), &runs);
					fLoggingView->ScrollToOffset(fLoggingView->TextLength());
				}
		
				if (fLogFileFlag) {
					if (pthread_rwlock_rdlock(&fLogFileLock) == 0) {
						fLogFile->Write(line.String(), line.Length());
						pthread_rwlock_unlock(&fLogFileLock);
					}
				}
			
				Unlock();
			}
		
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
PoorManWindow::FrameMoved(BPoint origin)
{
	fFrame.left = origin.x;
	fFrame.top = origin.y;
}


void 
PoorManWindow::FrameResized(float width, float height)
{
	if (fIsZoomed) {
		fLastWidth  = width;
		fLastHeight = height;
	}
}


bool
PoorManWindow::QuitRequested()
{
	if (fStatus) {
		time_t now = time(NULL);
		BString timeString;
		BLocale::Default()->FormatDateTime(&timeString, now, 
			DATE_FORMAT, TIME_FORMAT);
		
		BString line;
		line << "[" << timeString << "]: " << B_TRANSLATE("Shutting down.") 
			<< "\n";
		
		if (fLogConsoleFlag) {
			fLoggingView->Insert(fLoggingView->TextLength(),
				line, line.Length());
			fLoggingView->ScrollToOffset(fLoggingView->TextLength());
		}
		
		if (fLogFileFlag) {
			if (pthread_rwlock_rdlock(&fLogFileLock) == 0) {
				fLogFile->Write(line, line.Length());
				pthread_rwlock_unlock(&fLogFileLock);
			}
		}
		
		fServer->Stop();
		fStatus = false;
		UpdateStatusLabelAndMenuItem();
	}
	
	SaveSettings();
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	return true;
}


void
PoorManWindow::Zoom(BPoint origin, float width, float height)
{
	if (fIsZoomed) {
		// Change to the Minimal size
		fIsZoomed = false;
		ResizeTo(318, 53);
	} else {	
		// Change to the Zoomed size
		fIsZoomed = true;
		ResizeTo(fLastWidth, fLastHeight);
	}
}


void
PoorManWindow::SetHits(uint32 num)
{
	fHits = num;
	UpdateHitsLabel();
}


// Private: Methods ------------------------------------------


BMenu* 
PoorManWindow::BuildFileMenu() const
{
	BMenu* ptrFileMenu = new BMenu(STR_MNU_FILE);

	ptrFileMenu->AddItem(new BMenuItem(STR_MNU_FILE_SAVE_AS,
		new BMessage(MSG_MENU_FILE_SAVE_AS), CMD_FILE_SAVE_AS));
		
	ptrFileMenu->AddItem(new BMenuItem(STR_MNU_FILE_SAVE_SELECTION,
		new BMessage(MSG_MENU_FILE_SAVE_SELECTION)));
		
	ptrFileMenu->AddSeparatorItem();

	ptrFileMenu->AddItem(new BMenuItem(STR_MNU_FILE_QUIT,
		new BMessage(B_QUIT_REQUESTED), CMD_FILE_QUIT));
		
	return ptrFileMenu;
}


BMenu*	
PoorManWindow::BuildEditMenu() const
{
	BMenu* ptrEditMenu = new BMenu(STR_MNU_EDIT);
	
	BMenuItem* CopyMenuItem = new BMenuItem(STR_MNU_EDIT_COPY,
		new BMessage(B_COPY), CMD_EDIT_COPY);

	ptrEditMenu->AddItem(CopyMenuItem);
	CopyMenuItem->SetTarget(fLoggingView, NULL);
	
	ptrEditMenu->AddSeparatorItem();

	BMenuItem* SelectAllMenuItem = new BMenuItem(STR_MNU_EDIT_SELECT_ALL,
	new BMessage(B_SELECT_ALL), CMD_EDIT_SELECT_ALL);
	
	ptrEditMenu->AddItem(SelectAllMenuItem);
	SelectAllMenuItem->SetTarget(fLoggingView, NULL);
	
	ptrEditMenu->AddSeparatorItem();
	
	BMenuItem* PrefMenuItem = new BMenuItem(STR_MNU_EDIT_PREF,
		new BMessage(MSG_MENU_EDIT_PREF));
	ptrEditMenu->AddItem(PrefMenuItem);
	
	return ptrEditMenu;
}


BMenu*	
PoorManWindow::BuildControlsMenu() const
{
	BMenu* ptrControlMenu = new BMenu(STR_MNU_CTRL);
	
	BMenuItem* RunServerMenuItem = new BMenuItem(STR_MNU_CTRL_RUN_SERVER,
		new BMessage(MSG_MENU_CTRL_RUN));
	RunServerMenuItem->SetMarked(false);
	ptrControlMenu->AddItem(RunServerMenuItem);
	
	BMenuItem* ClearHitCounterMenuItem = new BMenuItem(STR_MNU_CTRL_CLEAR_HIT_COUNTER,
		new BMessage(MSG_MENU_CTRL_CLEAR_HIT));
	ptrControlMenu->AddItem(ClearHitCounterMenuItem);

	ptrControlMenu->AddSeparatorItem();

	BMenuItem* ClearConsoleLogMenuItem = new BMenuItem(STR_MNU_CTRL_CLEAR_CONSOLE,
		new BMessage(MSG_MENU_CTRL_CLEAR_CONSOLE));
	ptrControlMenu->AddItem(ClearConsoleLogMenuItem);

	BMenuItem* ClearLogFileMenuItem = new BMenuItem(STR_MNU_CTRL_CLEAR_LOG_FILE,
		new BMessage(MSG_MENU_CTRL_CLEAR_LOG));
	ptrControlMenu->AddItem(ClearLogFileMenuItem);
	
	return ptrControlMenu;
}


void 
PoorManWindow::SetDirLabel(const char* name)
{
	BString dirPath(B_TRANSLATE("Directory: "));
	dirPath.Append(name);
	
	if (Lock()) {
		fDirView->SetText(dirPath.String());
		Unlock();
	}
}


void 
PoorManWindow::UpdateStatusLabelAndMenuItem()
{
	if (Lock()) {
		if (fStatus)
			fStatusView->SetText(B_TRANSLATE("Status: Running"));
		else
			fStatusView->SetText(B_TRANSLATE("Status: Stopped"));
		fControlsMenu->FindItem(STR_MNU_CTRL_RUN_SERVER)->SetMarked(fStatus);
		Unlock();
	}
}


void 
PoorManWindow::UpdateHitsLabel()
{
	if (Lock()) {
		sprintf(fHitsLabel, B_TRANSLATE("Hits: %lu"), GetHits());
		fHitsView->SetText(fHitsLabel);
		
		Unlock();
	}
}


status_t 
PoorManWindow::SaveConsole(BMessage* message, bool selection)
{
	entry_ref	ref;
	const char* name;
	BPath		path;
	BEntry		entry;
	status_t	err = B_OK;
	FILE*		f;
	
	err = message->FindRef("directory", &ref);
	if (err != B_OK)
		return err;
	
	err = message->FindString("name", &name);
	if (err != B_OK)
		return err;
	
	err = entry.SetTo(&ref);
	if (err != B_OK)
		return err;
	
	entry.GetPath(&path);
	path.Append(name);
	
	if (!(f = fopen(path.Path(), "w")))
		return B_ERROR;
	
	if (!selection) {
		// write the data to the file
		err = fwrite(fLoggingView->Text(), 1, fLoggingView->TextLength(), f);
	} else {
		// find the selected text and write it to a file
		int32 start = 0, end = 0;
		fLoggingView->GetSelection(&start, &end);
		
		BString buffer;
		char * buffData = buffer.LockBuffer(end - start + 1);
		// copy the selected text from the TextView to the buffer	
		fLoggingView->GetText(start, end - start, buffData);
		buffer.UnlockBuffer(end - start + 1);
		
		err = fwrite(buffer.String(), 1, end - start + 1, f);
	}
	
	fclose(f);
	
	return err;
}


void
PoorManWindow::DefaultSettings()
{
	BAlert* serverAlert = new BAlert(B_TRANSLATE("Error Server"), 
		STR_ERR_CANT_START, B_TRANSLATE("OK"));
	BAlert* dirAlert = new BAlert(B_TRANSLATE("Error Dir"), 
		STR_ERR_WEB_DIR, B_TRANSLATE("Cancel"), B_TRANSLATE("Select"), 
		B_TRANSLATE("Default"), B_WIDTH_AS_USUAL, B_OFFSET_SPACING);
	dirAlert->SetShortcut(0, B_ESCAPE);
	int32 buttonIndex = dirAlert->Go();

	switch (buttonIndex) {
		case 0:
			if (Lock())
				Quit();
			be_app_messenger.SendMessage(B_QUIT_REQUESTED);
			break;
		
		case 1:
			fPrefWindow = new PoorManPreferencesWindow(
					fSetwindowFrame,
					STR_WIN_NAME_PREF);
			fPrefWindow->ShowWebDirFilePanel();
			break;

		case 2:
			if (create_directory(STR_DEFAULT_WEB_DIRECTORY, 0755) != B_OK) {
				serverAlert->Go();
				if (Lock())
					Quit();
				be_app_messenger.SendMessage(B_QUIT_REQUESTED);
				break;
			}
			BAlert* dirCreatedAlert =
				new BAlert(B_TRANSLATE("Dir Created"), STR_DIR_CREATED, 
					B_TRANSLATE("OK"));
			dirCreatedAlert->Go();
			SetWebDir(STR_DEFAULT_WEB_DIRECTORY);	
			be_app->PostMessage(kStartServer);
			break;
	}
}


status_t
PoorManWindow::ReadSettings()
{
	BPath p;
	BFile f;
	BMessage m;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &p) != B_OK)
		return B_ERROR;
	p.Append(STR_SETTINGS_FILE_NAME);
	
	f.SetTo(p.Path(), B_READ_ONLY);
	if (f.InitCheck() != B_OK)
		return B_ERROR;
	
	if (m.Unflatten(&f) != B_OK)
		return B_ERROR;
	
	if (MSG_PREF_FILE != m.what)
		return B_ERROR;
	
	//site tab
	if (m.FindString("fWebDirectory", &fWebDirectory) != B_OK)
		fWebDirectory.SetTo(STR_DEFAULT_WEB_DIRECTORY);
	if (m.FindString("fIndexFileName", &fIndexFileName) != B_OK)
		fIndexFileName.SetTo("index.html");
	if (m.FindBool("fDirListFlag", &fDirListFlag) != B_OK)
		fDirListFlag = false;
	
	//logging tab
	if (m.FindBool("fLogConsoleFlag", &fLogConsoleFlag) != B_OK)
		fLogConsoleFlag = true;
	if (m.FindBool("fLogFileFlag", &fLogFileFlag) != B_OK)
		fLogFileFlag = false;
	if (m.FindString("fLogPath", &fLogPath) != B_OK)
		fLogPath.SetTo("");
	
	//advance tab
	if (m.FindInt16("fMaxConnections", &fMaxConnections) != B_OK)
		fMaxConnections = (int16)32;
	
	//windows' position and size
	if (m.FindRect("frame", &fFrame) != B_OK)
		fFrame.Set(82.0f, 30.0f, 400.0f, 350.0f);
	if (m.FindRect("fSetwindowFrame", &fSetwindowFrame) != B_OK)
		fSetwindowFrame.Set(112.0f, 60.0f, 492.0f, 340.0f);
	if (m.FindBool("fIsZoomed", &fIsZoomed) != B_OK)
		fIsZoomed = true;
	if (m.FindFloat("fLastWidth", &fLastWidth) != B_OK)
		fLastWidth = 318.0f;
	if (m.FindFloat("fLastHeight", &fLastHeight) != B_OK)
		fLastHeight = 320.0f;
	
	fIsZoomed?ResizeTo(fLastWidth, fLastHeight):ResizeTo(318, 53);
	MoveTo(fFrame.left, fFrame.top);
	
	fLogFile = new BFile(fLogPath.String(), B_CREATE_FILE | B_WRITE_ONLY
		| B_OPEN_AT_END);
	if (fLogFile->InitCheck() != B_OK) {
		fLogFileFlag = false;
		//log it to console, "log to file unavailable."
		return B_OK;
	}
	
	SetDirLabel(fWebDirectory.String());
	
	return B_OK;
}


status_t
PoorManWindow::SaveSettings()
{
	BPath p;
	BFile f;
	BMessage m(MSG_PREF_FILE);
		
	//site tab
	m.AddString("fWebDirectory", fWebDirectory);
	m.AddString("fIndexFileName", fIndexFileName);
	m.AddBool("fDirListFlag", fDirListFlag);
	
	//logging tab
	m.AddBool("fLogConsoleFlag", fLogConsoleFlag);
	m.AddBool("fLogFileFlag", fLogFileFlag);
	m.AddString("fLogPath", fLogPath);
	
	//advance tab
	m.AddInt16("fMaxConnections", fMaxConnections);
	
	//windows' position and size
	m.AddRect("frame", fFrame);
	m.AddRect("fSetwindowFrame", fSetwindowFrame);
	m.AddBool("fIsZoomed", fIsZoomed);
	m.AddFloat("fLastWidth", fLastWidth);
	m.AddFloat("fLastHeight", fLastHeight);
		
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &p) != B_OK)
		return B_ERROR;
	p.Append(STR_SETTINGS_FILE_NAME);
	
	f.SetTo(p.Path(), B_WRITE_ONLY | B_ERASE_FILE | B_CREATE_FILE);
	if (f.InitCheck() != B_OK)
		return B_ERROR;
	
	if (m.Flatten(&f) != B_OK)
		return B_ERROR;

	return B_OK;
}


status_t
PoorManWindow::StartServer()
{
	if (fServer == NULL)
		fServer = new PoorManServer(fWebDirectory.String(), fMaxConnections,
			fDirListFlag, fIndexFileName.String());
	
	poorman_log(B_TRANSLATE("Starting up... "));
	if (fServer->Run() != B_OK) {
		return B_ERROR;
	}

	fStatus = true;
	UpdateStatusLabelAndMenuItem();
	poorman_log(B_TRANSLATE("done.\n"), false, INADDR_NONE, GREEN);
	
	return B_OK;
}


status_t
PoorManWindow::StopServer()
{
	if (fServer == NULL)
		return B_ERROR;
	
	poorman_log(B_TRANSLATE("Shutting down.\n"));
	fServer->Stop();
	fStatus = false;
	UpdateStatusLabelAndMenuItem();
	return B_OK;
}


void
PoorManWindow::SetLogPath(const char* str)
{
	if (!strcmp(fLogPath, str))
		return;
	
	BFile* temp = new BFile(str, B_CREATE_FILE | B_WRITE_ONLY | B_OPEN_AT_END);
	
	if (temp->InitCheck() != B_OK) {
		delete temp;
		return;
	}
	
	if (pthread_rwlock_wrlock(&fLogFileLock) == 0) {
		delete fLogFile;
		fLogFile = temp;
		pthread_rwlock_unlock(&fLogFileLock);
	} else {
		delete temp;
		return;
	}
	
	fLogPath.SetTo(str);
}
