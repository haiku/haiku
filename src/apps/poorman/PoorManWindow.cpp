/* PoorManWindow.cpp
 *
 *	Philip Harrison
 *	Started: 4/25/2004
 *	Version: 0.1
 */

#include "PoorManApplication.h"
#include "PoorManPreferencesWindow.h"
#include "PoorManWindow.h"
#include "constants.h"


#include <Box.h>
#include <Directory.h>
#include <Alert.h>

#include <stdio.h>


//#include <iostream>

PoorManWindow::PoorManWindow(BRect frame)
	: BWindow(frame, STR_APP_NAME, B_TITLED_WINDOW, 0),
	status(false), hits(0), last_width(325), last_height(155)
{	
	DefaultSettings();
	
	
	
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
	br = frame;
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
	statusRect.right = statusRect.left + 75;	// make the width wide enough for the string to display
		
	statusView = new BStringView(statusRect, "Status View", "Status: Stopped");
	bb->AddChild(statusView);
	
	// Directory String
	BRect dirRect;
	dirRect = Bounds();
	dirRect.top = statusRect.bottom - 1;
	dirRect.bottom = dirRect.top + 15;
	dirRect.left = statusRect.left;
	dirRect.right -= 5;
	
	dirView = new BStringView(dirRect, "Dir View", "Directory: /boot/home/", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	bb->AddChild(dirView);
	
	// Hits String
	BRect hitsRect;
	hitsRect = bb->Bounds();
	hitsRect.InsetBy(5, 5);
	hitsRect.top = statusRect.top;
	hitsRect.bottom = statusRect.bottom;
	hitsRect.left = statusRect.right + 20;
	
	hitsView = new BStringView(hitsRect, "Hit View", "Hits: 0", B_FOLLOW_RIGHT | B_FOLLOW_TOP);
	hitsView->SetAlignment(B_ALIGN_RIGHT);
	bb->AddChild(hitsView);

	// -----------------------------------------------------------------
	// Logging View
	
	// logRect
	BRect logRect(5.0, 36.0, 306.0, 131.0);
	
	// textRect	
	BRect textRect; //(1.0, 1.0, 175.0, 75.0);
	textRect = logRect;
	textRect.InsetBy(1.0, 1.0);
	textRect.top = 0.0;
	textRect.left = 0.0;

	loggingView = new BTextView(logRect, STR_TXT_VIEW, textRect,
		 B_FOLLOW_ALL_SIDES, B_WILL_DRAW );

	loggingView->MakeEditable(false);	// user cannot change the text
	loggingView->MakeSelectable(true);
	loggingView->SetViewColor(WHITE);

	loggingView->Insert("This is PoorMan.\n"); // text_run_array
	//loggingView->MakeFocus(true);
	//AddChild(loggingView);
	
	
	// create the scroll view
	scrollView = new BScrollView("Scroll View", loggingView, B_FOLLOW_ALL_SIDES, 
					B_WILL_DRAW | B_FRAME_EVENTS, 
					// Make sure articles on border do not occur when resizing
					false, true);
	bb->AddChild(scrollView);



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
	BWindow * change_title;
	
	saveConsoleFilePanel = new BFilePanel(
						B_SAVE_PANEL,
						new BMessenger(this),
						NULL,
						B_FILE_NODE,
						false,
						new BMessage(MSG_FILE_PANEL_SAVE_CONSOLE)
						);
	change_title = saveConsoleFilePanel->Window();
	change_title->SetTitle(STR_FILEPANEL_SAVE_CONSOLE);
	
	saveConsoleSelectionFilePanel = new BFilePanel(
						B_SAVE_PANEL,
						new BMessenger(this),
						NULL,
						B_FILE_NODE,
						false,
						new BMessage(MSG_FILE_PANEL_SAVE_CONSOLE_SELECTION)
						);
	change_title = saveConsoleSelectionFilePanel->Window();
	change_title->SetTitle(STR_FILEPANEL_SAVE_CONSOLE_SELECTION);
	
	
	AddChild(FileMenuBar);
}

void 
PoorManWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_MENU_FILE_SAVE_AS:
		saveConsoleFilePanel->Show();
		break;
	case MSG_FILE_PANEL_SAVE_CONSOLE:
		printf("FilePanel: Save Console\n");
		SaveConsole(message, false);
		break;
	case MSG_MENU_FILE_SAVE_SELECTION:
		saveConsoleSelectionFilePanel->Show();
		break;
	case MSG_FILE_PANEL_SAVE_CONSOLE_SELECTION:
		printf("FilePanel: Save Console Selection\n");
		SaveConsole(message, true);
		break;
	case MSG_MENU_EDIT_PREF:
		prefWindow = new PoorManPreferencesWindow(BRect(30.0f, 30.0f, 410.0f, 310.0f), STR_WIN_NAME_PREF);
		break;
	case MSG_MENU_CTRL_RUN:
		BMenuItem * RunServer;
		RunServer = ControlsMenu->FindItem(STR_MNU_CTRL_RUN_SERVER);
		if (RunServer)
		{
			//status = httpd->Run();
			/* For Testing 
			if (status)
				status = false;
			else
				status = true;
			*/
				
			UpdateStatusLabel(status);
			if (status)
			{
				RunServer->SetMarked(true);
			} else {
				RunServer->SetMarked(false);
			}
		}
		break;
	case MSG_MENU_CTRL_CLEAR_HIT:
		SetHits(0);
		UpdateHitsLabel();
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
	default:
		BWindow::MessageReceived(message);
		break;
	}
}


void 
PoorManWindow::FrameResized(float width, float height)
{
	if (is_zoomed)
	{
		last_width  = width;
		last_height = height;
	}
	printf("(%f, %f)\n", width, height);
}

bool 
PoorManWindow::QuitRequested()
{
	SaveSettings();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return (true);
}

void 
PoorManWindow::Zoom(BPoint origin, float width, float height)
{
	printf("Zoom: Is Zoomed before: %d (%f, %f)\n", is_zoomed, width, height); 
	if (is_zoomed)
	{	// Change to the Minimal size
		is_zoomed = false;
		ResizeTo(318, 53);
		printf("ResizedTo(318, 53)\n");
	}
	else
	{	// Change to the Zoomed size
		is_zoomed = true;
		ResizeTo(last_width, last_height);
		printf("ResizedTo(%f, %f)\n", last_width, last_height);
	}
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
	
	// about box
	BMenuItem * AboutItem = new BMenuItem(STR_MNU_FILE_ABOUT,
		new BMessage(B_ABOUT_REQUESTED));
	AboutItem->SetTarget(NULL, be_app);
	ptrFileMenu->AddItem(AboutItem);

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
	BString dirPath("Directory: ");
	int32 length = dirPath.Length() + 1;
	dirPath.LockBuffer(length);
	dirPath.Append(name);

	dirPath.UnlockBuffer(length + strlen(name));
	
	if (Lock())
	{
		dirView->SetText(dirPath.String());
		Unlock();
	}
}

void 
PoorManWindow::UpdateStatusLabel(bool set)
{
	if (Lock())
	{
		if (set)
			statusView->SetText("Status: Running");
		else
			statusView->SetText("Status: Stopped");
		
		Unlock();
	}
}

void 
PoorManWindow::UpdateHitsLabel()
{
	if (Lock())
	{
		sprintf(hitsLabel, "Hits: %lu", GetHits());
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
	
	if (err = message->FindRef("directory", &ref) != B_OK)
		return err;
	
	if (err = message->FindString("name", &name) != B_OK)
		return err;
	
	if (err = entry.SetTo(&ref) != B_OK)
		return err;
	
	entry.GetPath(&path);
	path.Append(name);
	
	if (!(f = fopen(path.Path(), "w")))
		return B_ERROR;
	
	if (!selection)
	{	
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
	// Site Tab
	SetIndexFileName("index.html");
	SetDirListFlag(false);
	// Logging Tab
	SetLogConsoleFlag(true);
	SetLogFileFlag(false);
	// Advanced Tab
	SetMaxConnections(32);

	BDirectory webDir;
	if (!webDir.Contains("/boot/home/public_html", B_DIRECTORY_NODE))
	{
		BAlert * serverAlert = new BAlert("Error Server", STR_ERR_CANT_START, "OK");

		BAlert * dirAlert = new BAlert("Error Dir", STR_ERR_WEB_DIR, 
			"Cancel", "Select", "Default", B_WIDTH_AS_USUAL, B_OFFSET_SPACING);
		dirAlert->SetShortcut(0, B_ESCAPE);
		int32 buttonIndex = dirAlert->Go();


		
		// process dirAlert
		switch (buttonIndex)
		{
			case 0:
				serverAlert->Go();
				QuitRequested();
				break;
			case 1:
				serverAlert->Go();
				prefWindow = new PoorManPreferencesWindow(BRect(30.0f, 30.0f, 410.0f, 310.0f), STR_WIN_NAME_PREF);
				prefWindow->ShowWebDirFilePanel();
				break;
			case 2:

				break;
		}
	} else {
		printf("BDirectory contains: /boot/home/public_html\n");
		SetWebDir("/boot/home/public_html");
	}
	
}

status_t 
PoorManWindow::ReadSettings()
{
}

status_t 
PoorManWindow::SaveSettings()
{
	FILE * 	f;
	size_t	s;
	
	f = fopen(STR_SETTINGS_NEW_FILE_PATH, "wb");
	if (f)
	{
		// Need to rewrite
		/*
		//fprintf(f, "%s",  web_directory.String());
		s = fwrite(web_directory.String(), sizeof(char), web_directory.Length() + 1, f);

		fprintf(f, "%s",  index_file_name.String());
		fwrite(dir_list_flag, sizeof(bool), 1, f); 
		//fprintf(f, "%uc", dir_list_flag);

		// logging tab
		fwrite(log_file_flag, sizeof(bool), 1, f);
		//fprintf(f, "%uc", log_console_flag);
		fprintf(f, "%uc", log_file_flag);
		fprintf(f, "%s",  log_path.String());
		// advanced tab
		fprintf(f, "%ld", max_connections);
	
		fprintf(f, "%uc", is_zoomed);
		fprintf(f, "%f",  last_width);
		fprintf(f, "%f",  last_height);
		*/
	}
	fclose(f);
}
