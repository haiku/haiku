#include <stdio.h>

#include <InterfaceKit.h>

#include "FileSelector.h"

FileSelector::FileSelector(void)
	: BWindow(BRect(0,0,320,160), "printtofile", B_TITLED_WINDOW,
	B_NOT_ZOOMABLE, B_CURRENT_WORKSPACE)
{
	m_exit_sem 		= create_sem(0, "FileSelector");
	m_result 		= B_ERROR;
	m_save_panel 	= NULL;
}

FileSelector::~FileSelector()
{
	delete m_save_panel;
	delete_sem(m_exit_sem);
}


bool FileSelector::QuitRequested()
{
	release_sem(m_exit_sem);
	return true;
}


void FileSelector::MessageReceived(BMessage * msg)
{
	switch (msg->what)
		{
		case START_MSG:
			m_save_panel = new BFilePanel(B_SAVE_PANEL, 
							new BMessenger(this), NULL, 0, false);

			m_save_panel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
			m_save_panel->Show();
			break;

		case B_SAVE_REQUESTED:
			{
			entry_ref 		dir;
			
			if ( msg->FindRef("directory", &dir) == B_OK)
				{
				const char *	name;

				BDirectory bdir(&dir);
				if ( msg->FindString("name", &name) == B_OK)
					{
					if ( name != NULL )
						m_result = m_entry.SetTo(&bdir, name);
					};
				};

			release_sem(m_exit_sem);
			break;
			};
		
		case B_CANCEL:
			release_sem(m_exit_sem);
			break;

		default:
			inherited::MessageReceived(msg);
			break;
		};
}
			

status_t FileSelector::Go(entry_ref * ref)
{
	MoveTo(300,300);
	Hide();
	Show();
	PostMessage(START_MSG);
	acquire_sem(m_exit_sem);

	if ( m_result == B_OK && ref)
		m_result = m_entry.GetRef(ref);

	Lock();
	Quit();

	return m_result;
}


			
