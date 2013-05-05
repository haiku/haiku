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


class PoorManPreferencesWindow: public BWindow {
public:
							PoorManPreferencesWindow(BRect frame, char* name);
							~PoorManPreferencesWindow();

	virtual	void			MessageReceived(BMessage* message);

			void			ShowWebDirFilePanel();
			void			SelectWebDir(BMessage* message);
			void			CreateLogFile(BMessage* message);

private:
			PoorManView*	fPrefView;
			PoorManView*	fButtonView;

			// ------------------------------------------------
			// Tabs
			BTabView*		fPrefTabView;
			BTab*			fSiteTab;
			BTab*			fLoggingTab;
			BTab*			fAdvancedTab;
			// Tab Views
			PoorManSiteView*		fSiteView;
			PoorManLoggingView*		fLoggingView;
			PoorManAdvancedView*	fAdvancedView;

			// ------------------------------------------------
			// Buttons
			BButton*		fCancelButton;
			BButton*		fDoneButton;

			// ------------------------------------------------
			// FilePanels
			BFilePanel*		fWebDirFilePanel;
			BFilePanel*		fLogFilePanel;


				// ------------------------------------------------
			// temporary preference variables used to save and
			// set the application to
			// site tab
			char			fWebDirectory[B_FILE_NAME_LENGTH];
			char			fIndexFileName[64];
			bool			fSendDir;
			// logging tab
			bool			flogToConsole;
			bool			fLogToFile;
			char			fLogFileName[B_FILE_NAME_LENGTH];
			// advanced tab
			int32			fMaxConnections;
};

#endif
