// license: public domain
// authors: jonas.sundstrom@kirilla.com

#include <Debug.h>

#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Roster.h>
#include <InterfaceKit.h>
#include <String.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Path.h>
#include <File.h>

#include "ZipOMatic.h"
#include "ZipOMaticActivity.h"
#include "ZipOMaticMisc.h"
#include "ZipOMaticView.h"
#include "ZipOMaticWindow.h"
#include "ZipOMaticZipper.h"

ZippoWindow::ZippoWindow(BMessage * a_message)
: BWindow(BRect(200,200,430,310), "Zip-O-Matic", B_TITLED_WINDOW, B_NOT_V_RESIZABLE), // | B_NOT_ZOOMABLE),
	m_zippo_settings				(),
	m_zipper_thread					(NULL),
	m_got_refs_at_window_startup	(false),
	m_zipping_was_stopped			(false),
	m_alert_invoker_message			(new BMessage ('alrt')),
	m_alert_window_invoker			(new BInvoker (m_alert_invoker_message, NULL, this))
{
	PRINT(("ZippoWindow()\n"));

//	Settings
	status_t	status	=	B_OK;

	status	=	m_zippo_settings.SetTo("ZipOMatic.msg");
	if (status != B_OK)	
		error_message("ZippoWindow() - m_zippo_settings.SetTo()", status);

	status	=	m_zippo_settings.InitCheck();
	if (status != B_OK)
		error_message("ZippoWindow() - m_zippo_settings.InitCheck()", status);
	
//	Interface
	zippoview  =  new ZippoView(Bounds());

	AddChild (zippoview);
	
	SetSizeLimits(Bounds().Width(), 15000, Bounds().Height(), Bounds().Height());

//	Settings, (on-screen location of window)
	ReadSettings();

//	Start zipper thread
	if (a_message != NULL)
	{
		m_got_refs_at_window_startup  =  true;
		StartZipping (a_message);
	}
}

ZippoWindow::~ZippoWindow()	
{
	PRINT(("ZippoWindow::~ZippoWindow()\n"));

	//delete m_alert_invoker_message;
	delete m_alert_window_invoker;

	// anything left to clean up?
}

void 
ZippoWindow::MessageReceived		(BMessage * a_message)
{
	switch(a_message->what)
	{

		case B_REFS_RECEIVED:
						StartZipping (a_message);
						break;

		case B_SIMPLE_DATA:
						if (IsZipping())
						{	
							a_message->what = B_REFS_RECEIVED;	
							be_app_messenger.SendMessage(a_message);
						}
						else
							StartZipping (a_message);
						break;
		
		case 'exit':	// thread has finished		(finished, quit, killed, we don't know)
						// reset window state
						{
							m_zipper_thread = NULL;
							zippoview->m_activity_view->Stop();
							zippoview->m_archive_name_view->SetText(" ");
							if (m_zipping_was_stopped)
								zippoview->m_zip_output_view->SetText("Stopped");
							else
								zippoview->m_zip_output_view->SetText("Archive created OK");
								
							CloseWindowOrKeepOpen();
							break;
						}
											
		case 'exrr':	// thread has finished
						// reset window state
						
						m_zipper_thread = NULL;
						zippoview->m_activity_view->Stop();
						zippoview->m_archive_name_view->SetText("");
						zippoview->m_zip_output_view->SetText("Error creating archive");
						//CloseWindowOrKeepOpen();
						break;
						
		case 'strt':
					{
						BString archive_filename;
						if (a_message->FindString("archive_filename", & archive_filename) == B_OK)
							zippoview->m_archive_name_view->SetText(archive_filename.String());		
						break;	
					}		

		case 'outp':
					{
						BString zip_output;
						if (a_message->FindString("zip_output", & zip_output) == B_OK)
							zippoview->m_zip_output_view->SetText(zip_output.String());		
						break;	
					}							
						
		case 'alrt':
					{
						int32	which_button	=	-1;
						if (a_message->FindInt32("which", & which_button) == B_OK)
							if (which_button == 0)
								StopZipping();
							else
							{
								if (m_zipper_thread != NULL)
									m_zipper_thread->ResumeExternalZip();
									
								zippoview->m_activity_view->Start();
							}
						break;	
					}
			
		default:	BWindow::MessageReceived(a_message);	break;			
	}
}

bool
ZippoWindow::QuitRequested	(void)
{
	PRINT(("ZippoWindow::QuitRequested()\n"));
	
	if (m_zipper_thread == NULL)
	{
		WriteSettings();
		be_app_messenger.SendMessage(ZIPPO_WINDOW_QUIT);
		return true;
	}
	else
	{
		if (m_zipper_thread != NULL)
			m_zipper_thread->SuspendExternalZip();
	
		zippoview->m_activity_view->Pause();
	
		BAlert * quit_requester = new BAlert ("Stop or Continue", "Are you sure you want to "
									"stop creating this archive?", "Stop", "Continue",
									 NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
		quit_requester->Go(m_alert_window_invoker);
		return false;
	}
}

status_t
ZippoWindow::ReadSettings	(void)
{
	status_t	status	=	B_OK;

	status	=	m_zippo_settings.InitCheck();
	if (status != B_OK)
		error_message("m_zippo_settings.InitCheck()", status);

	status	=	m_zippo_settings.ReadSettings();
	if (status != B_OK)
		error_message("m_zippo_settings.ReadSettings()", status);

	BRect	window_rect;
	
	status	=	m_zippo_settings.FindRect("window_rect", & window_rect);
	if (status != B_OK)
	{
		error_message("m_settings_message->FindRect(window_rect)", status);
		return status;
	}
	
	ResizeTo (window_rect.Width(), window_rect.Height());
	MoveTo (window_rect.LeftTop());
	
	return B_OK;
}

status_t
ZippoWindow::WriteSettings	(void)
{
	status_t	status	=	B_OK;

	status	=	m_zippo_settings.InitCheck();
	if (status != B_OK)
		error_message("m_zippo_settings.InitCheck()", status);

	status	=	m_zippo_settings.MakeEmpty();
	if (status != B_OK)
		error_message("m_zippo_settings.MakeEmpty()", status);

	status	=	m_zippo_settings.AddRect("window_rect", Frame());
	if (status != B_OK)
	{
		error_message("m_settings_message->AddRect(window_rect)", status);
		return status;
	}
	
	status	=	m_zippo_settings.WriteSettings();
	if (status != B_OK)
	{
		error_message("m_zippo_settings.WriteSettings()", status);
		return status;
	}

	
	return B_OK;
}

void
ZippoWindow::StartZipping (BMessage * a_message)
{
	PRINT(("ZippoWindow::StartZipping()\n"));
	
	zippoview->m_stop_button->SetEnabled(true);
	zippoview->m_activity_view->Start();

	m_zipper_thread	=	new ZipperThread (a_message, this);
	m_zipper_thread->Start();

	m_zipping_was_stopped  =  false;
}

void
ZippoWindow::StopZipping (void)
{
	PRINT(("ZippoWindow::StopZipping()\n"));

	m_zipping_was_stopped  =  true;

	zippoview->m_stop_button->SetEnabled(false);
	
	zippoview->m_activity_view->Stop();

	m_zipper_thread->InterruptExternalZip();
	m_zipper_thread->Quit();
	
	status_t  status  =  B_OK;
	m_zipper_thread->WaitForThread (& status);
	m_zipper_thread  =  NULL;

	zippoview->m_archive_name_view->SetText(" ");
	zippoview->m_zip_output_view->SetText("Stopped");

	CloseWindowOrKeepOpen();
}

bool
ZippoWindow::IsZipping (void)
{
	if (m_zipper_thread == NULL)
		return false;
	else
		return true;
}

void
ZippoWindow::CloseWindowOrKeepOpen (void)
{
	if (m_got_refs_at_window_startup)
		PostMessage(B_QUIT_REQUESTED);
}

void
ZippoWindow::Zoom (BPoint origin, float width, float height)
{
	/*
	float archive_name_view_preferred_width;
	float zip_output_view_preferred_width;
	float throw_away_height;
	zippoview->GetPreferredSize(& archive_name_view_preferred_width, & throw_away_height);
	zippoview->GetPreferredSize(& zip_output_view_preferred_width, & throw_away_height);
	*/

	//	BStringView::GetPreferredSize appears to be broken,
	//	so we have to use BView::StringWidth() instead

	if (IsZipping())
	{
		float archive_name_view_preferred_width  =  zippoview->StringWidth(zippoview->m_archive_name_view->Text());
		float zip_output_view_preferred_width  =  zippoview->StringWidth(zippoview->m_zip_output_view->Text());
	
		float the_wide_string;
	
		if (zip_output_view_preferred_width > archive_name_view_preferred_width)
			the_wide_string  =  zip_output_view_preferred_width;
		else
			the_wide_string  =  archive_name_view_preferred_width;
		
		ResizeTo(the_wide_string, Bounds().Height());
	}
	else
	{
		ResizeTo(230,110);
	}
}

