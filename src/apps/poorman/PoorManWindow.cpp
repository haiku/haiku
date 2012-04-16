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
	: BWindow(frame, STR_APP_NAME, B_TITLED_WINDOW, 0),
	status(false), hits(0), prefWindow(NULL), fLogFile(NULL), fServer(NULL)
{
	//preferences init
	web_directory.SetTo(STR_DEFAULT_WEB_DIRECTORY);
	index_file_name.SetTo("index.html");
	dir_list_flag = false;
	
	log_console_flag = true;
	log_file_flag = false;
	log_path.SetTo("");
	
	max_connections = (int16)32;
	
	is_zoomed = true;
	last_width = 318.0f;
	last_height = 320.0f;
	this->frame = frame;
	setwindow_frame.Set(112.0f, 60.0f, 492.0f, 340.0f);
	
	// PoorMan Window
	SetSizeLimits(318, 1600, 53, 1200); 
	// limit the size of the size of the window
	
	//SetZoomLimits(1024, 768);
	
	//frame.Set(30.0f, 30.0f, 355.0f, 185.0f);
	frame.OffsetTo(B_ORIGIN);
	frame = Bounds();
	frame.top += 19.0;
	
	mainView = new PoorManView(frame, STR_APP_NAME);
	mainView->SetViewColor(216,216,216,255);
	
	mainView->SetFont(be_bold_font);
	mainView->SetFontSize(12);
	AddChild(mainView);
	
	// BBox tests
	BRect br;
	br = mainView->Bounds();
	br.top = 1.0;

	BBox * bb = new BBox(br, "Background", B_FOLLOW_ALL_SIDES, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
	bb->SetHighColor(WHITE);
	bb->SetLowColor(GRAY);
	bb->SetBorder(B_PLAIN_BORDER);
	mainView->AddChild(bb);

	// -----------------------------------------------------------------
	// Three Labels 
	
	// Status String
	BRect statusRect;
	statusRect = Bounds();
	statusRect.left	+= 5;
	statusRect.top 	+= 3;
	statusRect.bottom = statusRect.top + 15;
	statusRect.right = statusRect.left + 100;	// make the width wide enough for the string to display
		
	statusView = new BStringView(statusRect, "Status View", 
		B_TRANSLATE("Status: Stopped"));
	bb->AddChild(statusView);
	
	// Directory String
	BRect dirRect;
	dirRect = Bounds();
	dirRect.top = statusRect.bottom - 1;
	dirRect.bottom = dirRect.top + 15;
	dirRect.left = statusRect.left;
	dirRect.right -= 5;
	
	dirView = new BStringView(dirRect, "Dir View", 
		B_TRANSLATE("Directory: (none)"), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	bb->AddChild(dirView);
	
	// Hits String
	BRect hitsRect;
	hitsRect = bb->Bounds();
	hitsRect.InsetBy(5.0f, 5.0f);
	hitsRect.top = statusRect.top;
	hitsRect.bottom = statusRect.bottom;
	hitsRect.left = statusRect.right + 20;
	
	hitsView = new BStringView(hitsRect, "Hit View", B_TRANSLATE("Hits: 0"), 
		B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	hitsView->SetAlignment(B_ALIGN_RIGHT);
	bb->AddChild(hitsView);

	// -----------------------------------------------------------------
	// Logging View
	
	// logRect
	BRect logRect = bb->Bounds();//(5.0, 36.0, 306.0, 131.0);
	logRect.InsetBy(5, 5);
	logRect.top = 36.0f;
	logRect.right -= B_V_SCROLL_BAR_WIDTH;
	
	// textRect	
	BRect textRect; //(1.0, 1.0, 175.0, 75.0);
	textRect = logRect;
	textRect.top = 0.0;
	textRect.left = 2.0;
	textRect.right = logRect.right - logRect.left - 2.0;
	textRect.bottom = logRect.bottom - logRect.top;

	fLogViewFont = new BFont(be_plain_font);
	fLogViewFont->SetSize(11.0);

	loggingView = new BTextView(logRect, STR_TXT_VIEW, textRect,
		 fLogViewFont, NULL, B_FOLLOW_ALL_SIDES, B_WILL_DRAW );

	loggingView->MakeEditable(false);	// user cannot change the text
	loggingView->MakeSelectable(true);
	loggingView->SetViewColor(WHITE);
	loggingView->SetStylable(true);
		
	// create the scroll view
	scrollView = new BScrollView("Scroll View", loggingView, B_FOLLOW_ALL_SIDES, 
					B_WILL_DRAW | B_FRAME_EVENTS, 
					// Make sure articles on border do not occur when resizing
					false, true);
	bb->AddChild(scrollView);
	loggingView->MakeFocus(true);


	// -----------------------------------------------------------------
	// menu bar
	BRect menuRect;
	menuRect = Bounds();
	menuRect.bottom = 18.0f;
	
	FileMenuBar = new BMenuBar(menuRect, "File Menu Bar");
	
	// menus
	FileMenu = BuildFileMenu();
	if (FileMenu)
		FileMenuBar->AddItem(FileMenu);
			
	EditMenu = BuildEditMenu();
	if (EditMenu)
		FileMenuBar->AddItem(EditMenu);
		
	ControlsMenu = BuildControlsMenu();
	if (ControlsMenu)
		FileMenuBar->AddItem(ControlsMenu);
	
	// File Panels
	BWindow* change_title;
	
	BMessenger messenger(this);
	saveConsoleFilePanel = new BFilePanel(
						B_SAVE_PANEL,
						&messenger,
						NULL,
						B_FILE_NODE,
						false,
						new BMessage(MSG_FILE_PANEL_SAVE_CONSOLE));

	change_title = saveConsoleFilePanel->Window();
	change_title->SetTitle(STR_FILEPANEL_SAVE_CONSOLE);
	
	saveConsoleSelectionFilePanel = new BFilePanel(
						B_SAVE_PANEL,
						&messenger,
						NULL,
						B_FILE_NODE,
						false,
						new BMessage(MSG_FILE_PANEL_SAVE_CONSOLE_SELECTION));

	change_title = saveConsoleSelectionFilePanel->Window();
	change_title->SetTitle(STR_FILEPANEL_SAVE_CONSOLE_SELECTION);
	
	
	AddChild(FileMenuBar);
	
	pthread_rwlock_init(&fLogFileLock, NULL);
}


PoorManWindow::~PoorManWindow()
{
	delete fServer;
	delete fLogViewFont;
	delete fLogFile;
	pthread_rwlock_destroy(&fLogFileLock);
}


void
PoorManWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_MENU_FILE_SAVE_AS:
		saveConsoleFilePanel->Show();
		break;
	case MSG_FILE_PANEL_SAVE_CONSOLE:
		printf("FilePanel: Save console\n");
		SaveConsole(message, false);
		break;
	case MSG_MENU_FILE_SAVE_SELECTION:
		saveConsoleSelectionFilePanel->Show();
		break;
	case MSG_FILE_PANEL_SAVE_CONSOLE_SELECTION:
		printf("FilePanel: Save console selection\n");
		SaveConsole(message, true);
		break;
	case MSG_FILE_PANEL_SELECT_WEB_DIR:		
		prefWindow->MessageReceived(message);
		break;
	case MSG_MENU_EDIT_PREF:
		prefWindow = new PoorManPreferencesWindow(
			setwindow_frame,
			STR_WIN_NAME_PREF);
		prefWindow->Show();
		break;
	case MSG_MENU_CTRL_RUN:
		if (status)
			StopServer();
		else
			StartServer();
		break;
	case MSG_MENU_CTRL_CLEAR_HIT:
		SetHits(0);
		//UpdateHitsLabel();
		break;
	case MSG_MENU_CTRL_CLEAR_CONSOLE:
		loggingView->SelectAll();
		loggingView->Delete();
		break;
	case MSG_MENU_CTRL_CLEAR_LOG:
		FILE * f;
		f = fopen(log_path.String(), "w");
		fclose(f);
		break;
	case MSG_LOG: {	
		if (!log_console_flag && !log_file_flag)
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
		run.font = *fLogViewFont;
		run.color = color;
		
		runs.count = 1;
		runs.runs[0] = run;
		
		if (Lock()) {
			if (log_console_flag) {
				loggingView->Insert(loggingView->TextLength(),
					line.String(), line.Length(), &runs);
				loggingView->ScrollToOffset(loggingView->TextLength());
			}
		
			if (log_file_flag) {
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
	frame.left = origin.x;
	frame.top = origin.y;
}


void 
PoorManWindow::FrameResized(float width, float height)
{
	if (is_zoomed) {
		last_width  = width;
		last_height = height;
	}
}


bool
PoorManWindow::QuitRequested()
{
	if (status) {
		time_t now = time(NULL);
		BString timeString;
		BLocale::Default()->FormatDateTime(&timeString, now, 
			DATE_FORMAT, TIME_FORMAT);
		
		BString line;
		line << "[" << timeString << "]: " << B_TRANSLATE("Shutting down.") 
			<< "\n";
		
		if (log_console_flag) {
				loggingView->Insert(loggingView->TextLength(),
					line, line.Length());
				loggingView->ScrollToOffset(loggingView->TextLength());
		}
		
		if (log_file_flag) {
			if (pthread_rwlock_rdlock(&fLogFileLock) == 0) {
					fLogFile->Write(line, line.Length());
					pthread_rwlock_unlock(&fLogFileLock);
			}
		}
		
		fServer->Stop();
		status = false;
		UpdateStatusLabelAndMenuItem();
	}
	
	SaveSettings();
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	return true;
}


void
PoorManWindow::Zoom(BPoint origin, float width, float height)
{
	if (is_zoomed) {
		// Change to the Minimal size
		is_zoomed = false;
		ResizeTo(318, 53);
	} else {	
		// Change to the Zoomed size
		is_zoomed = true;
		ResizeTo(last_width, last_height);
	}
}


void
PoorManWindow::SetHits(uint32 num)
{
	hits = num;
	UpdateHitsLabel();
}


// Private: Methods ------------------------------------------


BMenu * 
PoorManWindow::BuildFileMenu() const
{
	BMenu * ptrFileMenu = new BMenu(STR_MNU_FILE);

	ptrFileMenu->AddItem(new BMenuItem(STR_MNU_FILE_SAVE_AS,
		new BMessage(MSG_MENU_FILE_SAVE_AS), CMD_FILE_SAVE_AS));
		
	ptrFileMenu->AddItem(new BMenuItem(STR_MNU_FILE_SAVE_SELECTION,
		new BMessage(MSG_MENU_FILE_SAVE_SELECTION)));
		
	ptrFileMenu->AddSeparatorItem();

	ptrFileMenu->AddItem(new BMenuItem(STR_MNU_FILE_QUIT,
		new BMessage(B_QUIT_REQUESTED), CMD_FILE_QUIT));
		
	return ptrFileMenu;
}


BMenu *	
PoorManWindow::BuildEditMenu() const
{
	BMenu * ptrEditMenu = new BMenu(STR_MNU_EDIT);
	
	BMenuItem * CopyMenuItem = new BMenuItem(STR_MNU_EDIT_COPY,
		new BMessage(B_COPY), CMD_EDIT_COPY);

	ptrEditMenu->AddItem(CopyMenuItem);
	CopyMenuItem->SetTarget(loggingView, NULL);
	
	ptrEditMenu->AddSeparatorItem();

	BMenuItem * SelectAllMenuItem = new BMenuItem(STR_MNU_EDIT_SELECT_ALL,
	new BMessage(B_SELECT_ALL), CMD_EDIT_SELECT_ALL);
	
	ptrEditMenu->AddItem(SelectAllMenuItem);
	SelectAllMenuItem->SetTarget(loggingView, NULL);
	
	ptrEditMenu->AddSeparatorItem();
	
	BMenuItem * PrefMenuItem = new BMenuItem(STR_MNU_EDIT_PREF,
		new BMessage(MSG_MENU_EDIT_PREF));
	ptrEditMenu->AddItem(PrefMenuItem);
	
	return ptrEditMenu;
}


BMenu *	
PoorManWindow::BuildControlsMenu() const
{
	BMenu * ptrControlMenu = new BMenu(STR_MNU_CTRL);
	
	BMenuItem * RunServerMenuItem = new BMenuItem(STR_MNU_CTRL_RUN_SERVER,
		new BMessage(MSG_MENU_CTRL_RUN));
	RunServerMenuItem->SetMarked(false);
	ptrControlMenu->AddItem(RunServerMenuItem);
	
	BMenuItem * ClearHitCounterMenuItem = new BMenuItem(STR_MNU_CTRL_CLEAR_HIT_COUNTER,
		new BMessage(MSG_MENU_CTRL_CLEAR_HIT));
	ptrControlMenu->AddItem(ClearHitCounterMenuItem);

	ptrControlMenu->AddSeparatorItem();

	BMenuItem * ClearConsoleLogMenuItem = new BMenuItem(STR_MNU_CTRL_CLEAR_CONSOLE,
		new BMessage(MSG_MENU_CTRL_CLEAR_CONSOLE));
	ptrControlMenu->AddItem(ClearConsoleLogMenuItem);

	BMenuItem * ClearLogFileMenuItem = new BMenuItem(STR_MNU_CTRL_CLEAR_LOG_FILE,
		new BMessage(MSG_MENU_CTRL_CLEAR_LOG));
	ptrControlMenu->AddItem(ClearLogFileMenuItem);
	
	return ptrControlMenu;
}


void 
PoorManWindow::SetDirLabel(const char * name)
{
	BString dirPath(B_TRANSLATE("Directory: "));
	dirPath.Append(name);
	
	if (Lock()) {
		dirView->SetText(dirPath.String());
		Unlock();
	}
}


void 
PoorManWindow::UpdateStatusLabelAndMenuItem()
{
	if (Lock()) {
		if (status)
			statusView->SetText(B_TRANSLATE("Status: Running"));
		else
			statusView->SetText(B_TRANSLATE("Status: Stopped"));
		ControlsMenu->FindItem(STR_MNU_CTRL_RUN_SERVER)->SetMarked(status);
		Unlock();
	}
}


void 
PoorManWindow::UpdateHitsLabel()
{
	if (Lock()) {
		sprintf(hitsLabel, B_TRANSLATE("Hits: %lu"), GetHits());
		hitsView->SetText(hitsLabel);
		
		Unlock();
	}
}


status_t 
PoorManWindow::SaveConsole(BMessage * message, bool selection)
{
	entry_ref	ref;
	const char	* name;
	BPath		path;
	BEntry		entry;
	status_t	err = B_OK;
	FILE		*f;
	
	if ((err = message->FindRef("directory", &ref)) != B_OK)
		return err;
	
	if ((err = message->FindString("name", &name)) != B_OK)
		return err;
	
	if ((err = entry.SetTo(&ref)) != B_OK)
		return err;
	
	entry.GetPath(&path);
	path.Append(name);
	
	if (!(f = fopen(path.Path(), "w")))
		return B_ERROR;
	
	if (!selection) {
		// write the data to the file
		err = fwrite(loggingView->Text(), 1, loggingView->TextLength(), f);
	} else {
		// find the selected text and write it to a file
		int32 start = 0, end = 0;
		loggingView->GetSelection(&start, &end);
		
		BString buffer;
		char * buffData = buffer.LockBuffer(end - start + 1);
		// copy the selected text from the TextView to the buffer	
		loggingView->GetText(start, end - start, buffData);
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
			prefWindow = new PoorManPreferencesWindow(
					setwindow_frame,
					STR_WIN_NAME_PREF);
			prefWindow->ShowWebDirFilePanel();
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
	if (m.FindString("web_directory", &web_directory) != B_OK)
		web_directory.SetTo(STR_DEFAULT_WEB_DIRECTORY);
	if (m.FindString("index_file_name", &index_file_name) != B_OK)
		index_file_name.SetTo("index.html");
	if (m.FindBool("dir_list_flag", &dir_list_flag) != B_OK)
		dir_list_flag = false;
	
	//logging tab
	if (m.FindBool("log_console_flag", &log_console_flag) != B_OK)
		log_console_flag = true;
	if (m.FindBool("log_file_flag", &log_file_flag) != B_OK)
		log_file_flag = false;
	if (m.FindString("log_path", &log_path) != B_OK)
		log_path.SetTo("");
	
	//advance tab
	if (m.FindInt16("max_connections", &max_connections) != B_OK)
		max_connections = (int16)32;
	
	//windows' position and size
	if (m.FindRect("frame", &frame) != B_OK)
		frame.Set(82.0f, 30.0f, 400.0f, 350.0f);
	if (m.FindRect("setwindow_frame", &setwindow_frame) != B_OK)
		setwindow_frame.Set(112.0f, 60.0f, 492.0f, 340.0f);
	if (m.FindBool("is_zoomed", &is_zoomed) != B_OK)
		is_zoomed = true;
	if (m.FindFloat("last_width", &last_width) != B_OK)
		last_width = 318.0f;
	if (m.FindFloat("last_height", &last_height) != B_OK)
		last_height = 320.0f;
	
	is_zoomed?ResizeTo(last_width, last_height):ResizeTo(318, 53);
	MoveTo(frame.left, frame.top);
	
	fLogFile = new BFile(log_path.String(), B_CREATE_FILE | B_WRITE_ONLY
		| B_OPEN_AT_END);
	if (fLogFile->InitCheck() != B_OK) {
		log_file_flag = false;
		//log it to console, "log to file unavailable."
		return B_OK;
	}
	
	SetDirLabel(web_directory.String());
	
	return B_OK;
}


status_t
PoorManWindow::SaveSettings()
{
	BPath p;
	BFile f;
	BMessage m(MSG_PREF_FILE);
		
	//site tab
	m.AddString("web_directory", web_directory);
	m.AddString("index_file_name", index_file_name);
	m.AddBool("dir_list_flag", dir_list_flag);
	
	//logging tab
	m.AddBool("log_console_flag", log_console_flag);
	m.AddBool("log_file_flag", log_file_flag);
	m.AddString("log_path", log_path);
	
	//advance tab
	m.AddInt16("max_connections", max_connections);
	
	//windows' position and size
	m.AddRect("frame", frame);
	m.AddRect("setwindow_frame", setwindow_frame);
	m.AddBool("is_zoomed", is_zoomed);
	m.AddFloat("last_width", last_width);
	m.AddFloat("last_height", last_height);
		
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
		fServer = new PoorManServer(
			web_directory.String(),
			max_connections,
			dir_list_flag,
			index_file_name.String());
	
	poorman_log(B_TRANSLATE("Starting up... "));
	if (fServer->Run() != B_OK) {
		return B_ERROR;
	}

	status = true;
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
	status = false;
	UpdateStatusLabelAndMenuItem();
	return B_OK;
}


void
PoorManWindow::SetLogPath(const char* str)
{
	if (!strcmp(log_path, str))
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
	
	log_path.SetTo(str);
}
