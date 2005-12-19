// license: public domain
// authors: jonas.sundstrom@kirilla.com

#include <TrackerAddOn.h>
#include <Roster.h>
#include <Debug.h>

#include "ZipOMatic.h"
#include "ZipOMaticMisc.h"
#include "ZipOMaticWindow.h"

extern "C" void 
process_refs(entry_ref dir_ref, BMessage * msg, void *)
{
	msg->AddRef("dir_ref", & dir_ref);

	status_t	status		=	B_OK;	
	type_code	ref_type	=	B_REF_TYPE;
	int32		ref_count	=	0;
	
	status  =  msg->GetInfo("refs", & ref_type, & ref_count);
	if (status != B_OK || ref_count < 1)
		be_roster->Launch (ZIPOMATIC_APP_SIG);
	else
		be_roster->Launch (ZIPOMATIC_APP_SIG, msg );
}

int main()
{
	ZipOMatic app;
	app.Run();

	return (0);
}

ZipOMatic::ZipOMatic  (void)
 :	BApplication			(ZIPOMATIC_APP_SIG),
 	m_got_refs				(false)
{
	PRINT(("ZipOMatic::ZipOMatic()\n"));
	
	// void
}

ZipOMatic::~ZipOMatic  (void)	
{
	PRINT(("ZipOMatic::~ZipOMatic()\n"));
	
	fflush(stdout);
}

void 
ZipOMatic::RefsReceived  (BMessage * a_message)
{
	PRINT(("ZipOMatic::RefsReceived()\n"));

	if (IsLaunching())
		m_got_refs  =  true;
	
	BMessage * msg = new BMessage (* a_message);

	UseExistingOrCreateNewWindow(msg);
}

void
ZipOMatic::ReadyToRun  (void)
{
	PRINT(("ZipOMatic::ReadyToRun()\n"));
	
	if (m_got_refs)
	{
		// nothing - wait on window(s) to finish
	}
	else
		UseExistingOrCreateNewWindow();
}

void 
ZipOMatic::MessageReceived  (BMessage * a_message)
{
	PRINT(("ZipOMatic::MessageReceived()\n"));
	
	switch(a_message->what)
	{
		case ZIPPO_WINDOW_QUIT:
		
				snooze (200000);
				if (CountWindows() == 0)
					Quit();
				break;
					
		case B_SILENT_RELAUNCH:
		
				SilentRelaunch();
				break;
				
		default:	BApplication::MessageReceived(a_message);	break;			
	}
	
}

bool
ZipOMatic::QuitRequested  (void)
{
	// intelligent (?) closing of the windows
	//
	// overriding BApplication::QuitRequested();
	
	if (CountWindows() <= 0)
		return true;
	
	BList	window_list	(5);
	int32	window_count  =  0;
	BWindow	*	window;
	
	// build list of windows
	while (1)
	{
		window =  WindowAt(window_count++);
		if (window == NULL)
			break;
			
		window_list.AddItem(window);
	}
	
	// ask windows to quit
	while (1)
	{
		window = (BWindow *) window_list.RemoveItem(int32(0));
		if (window == NULL)
			break;
		
		if (window->Lock())
		{
			window->PostMessage(B_QUIT_REQUESTED);
			window->Unlock();
		}
	}

	PRINT(("CountWindows(): %ld\n", CountWindows()));

	if (CountWindows() <= 0)
		return true;
	
	return false; 	// default: stay alive
}

void
ZipOMatic::SilentRelaunch  (void)
{
	UseExistingOrCreateNewWindow();
}

void
ZipOMatic::UseExistingOrCreateNewWindow  (BMessage * a_message)
{
	int32      		window_count  =  0;
	ZippoWindow *	window;
	bool			found_non_busy_window	=	false;

	while (1)
	{
		window = dynamic_cast<ZippoWindow *>(WindowAt(window_count++));
		if (window == NULL)
			break;
		
		if (window->Lock())
		{
			if (! window->IsZipping())
			{
				found_non_busy_window  =  true;
				if (a_message != NULL)
					window->PostMessage(a_message);
				window->Activate();
				window->Unlock();
				break;
			}
			window->Unlock();
		}
	}
	
	if (! found_non_busy_window)
	{
		ZippoWindow	*	m_window	=	new ZippoWindow(a_message);
		m_window->Show();
	}
}
