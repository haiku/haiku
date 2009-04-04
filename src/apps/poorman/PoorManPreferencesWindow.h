/* PoorManPreferencesWindow.h
 *
 *	Philip Harrison
 *	Started: 4/27/2004
 *	Version: 0.1
 */


#ifndef POOR_MAN_PREFERENCES_WINDOW_H
#define POOR_MAN_PREFERENCES_WINDOW_H

#include <Window.h>
#include <SupportDefs.h>
#include <TabView.h>
#include <Path.h>
#include <FilePanel.h>
#include <Button.h>

#include "PoorManView.h"
#include "PoorManSiteView.h"
#include "PoorManLoggingView.h"
#include "PoorManAdvancedView.h"




class PoorManPreferencesWindow: public BWindow
{
private:

				PoorManView	*	prefView;
				PoorManView	*	buttonView;

				// ------------------------------------------------
				// Tabs
				BTabView	*	prefTabView;
				BTab		*	siteTab;
				BTab		*	loggingTab;
				BTab		*	advancedTab;
				// Tab Views
				PoorManSiteView		*	siteView;
				PoorManLoggingView	*	loggingView;
				PoorManAdvancedView	*	advancedView;

				// ------------------------------------------------
				// Buttons
				BButton		*	cancelButton;
				BButton		*	doneButton;

				// ------------------------------------------------
				// FilePanels
				BFilePanel	*	webDirFilePanel;
				BFilePanel	*	logFilePanel;


				// ------------------------------------------------
				// temporary preference variables used to save and
				// set the application to
				// site tab
				char		web_directory[B_FILE_NAME_LENGTH];
				char		index_file_name[64];
				bool		send_dir;
				// logging tab
				bool		log_to_console;
				bool		log_to_file;
				char		log_file_name[B_FILE_NAME_LENGTH];
				// advanced tab
				int32		max_connections;
public:
				PoorManPreferencesWindow(BRect frame, char * name);
				~PoorManPreferencesWindow();

virtual	void	MessageReceived(BMessage * message);

		void	ShowWebDirFilePanel();
		void	SelectWebDir(BMessage * message);
		void	CreateLogFile(BMessage * message);
};

#endif
