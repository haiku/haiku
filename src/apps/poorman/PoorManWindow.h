/* PoorManWindow.h
 *
 *	Philip Harrison
 *	Started: 4/25/2004
 *	Version: 0.1
 */

#ifndef POOR_MAN_WINDOW_H
#define POOR_MAN_WINDOW_H

#include <Window.h>
#include <Point.h>
#include <FilePanel.h>
#include <Path.h>
#include <Message.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuItem.h>
#include <TextView.h>
#include <StringView.h>
#include <ScrollView.h>
#include <String.h>
#include <SupportDefs.h>

#include "PoorManView.h"
#include "PoorManPreferencesWindow.h"

class PoorManWindow: public BWindow
{
public:
					PoorManWindow(BRect frame);
virtual	void		MessageReceived(BMessage * message);

virtual	void		FrameResized(float width, float height);
virtual	bool		QuitRequested();
virtual	void		Zoom(BPoint origin, float width, float height);

	// -------------------------------------------
	// Public PoorMan Window Methods
		void		SetDirLabel(const char * name);
		void		SetHits(uint32 num) { hits = num;}
		uint32		GetHits() { return hits; }
		status_t	SaveConsole(BMessage * message, bool);
		status_t	SaveSettings();
		
	// -------------------------------------------	
	// Preferences and Settings
		// Site Tab		
		bool		 DirListFlag() 				  { return dir_list_flag; }
		void		 SetDirListFlag(bool flag) 	  { dir_list_flag = flag; }
		const char * IndexFileName()			  { return index_file_name.String(); }
		void		 SetIndexFileName(const char * str) { index_file_name.SetTo(str); }
		const char * WebDir()					  { return web_directory.String(); }
		void		 SetWebDir(const char * str)  { web_directory.SetTo(str); }
		// Logging Tab
		bool		 LogConsoleFlag()			  { return log_console_flag; }
		void		 SetLogConsoleFlag(bool flag) { log_console_flag = flag; }
		bool		 LogFileFlag()				  { return log_file_flag; }
		void		 SetLogFileFlag(bool flag) 	  { log_file_flag = flag; }
		const char * LogPath()					  { return log_path.String(); }
		void		 SetLogPath(const char * str) { log_path.SetTo(str); }
		// Advanced Tab
		int32		 MaxConnections()			  { return max_connections; }
		void		 SetMaxConnections(int32 num) { max_connections = num;  }


private:
	// -------------------------------------------
	// PoorMan Window Methods
		status_t	ReadSettings();
		void		DefaultSettings();
		
		void		UpdateStatusLabel(bool);
		void		UpdateHitsLabel();
	
private:
	// -------------------------------------------
	// PoorMan Window
	PoorManView		* mainView;
	
	// -------------------------------------------
	// Build Menu Methods
	BMenu *		BuildFileMenu() const;
	BMenu *		BuildEditMenu() const;
	BMenu *		BuildControlsMenu() const;
	
	// --------------------------------------------
	// MenuBar & Menu items
	BMenuBar *	FileMenuBar;
	BMenu	 *	FileMenu;
	BMenu	 *	EditMenu;
	BMenu	 *	ControlsMenu;
	
	// --------------------------------------------
	// Status, Hits, Directory
	BStringView	*	statusView;
	BStringView	*	hitsView;
	BStringView	*	dirView;
	
	bool			status;
	uint32			hits;
	char			hitsLabel[25];
	
	// --------------------------------------------
	// Logging View
	BScrollView	*	scrollView;
	BTextView	*	loggingView;
	// use asctime() for format of [Date/Time]:
	

	// -------------------------------------------
	// PoorMan Preference Window
	PoorManPreferencesWindow * prefWindow;
	
	// site tab
	BString			web_directory;
	BString			index_file_name;
	bool			dir_list_flag;

	// logging tab
	bool			log_console_flag;
	bool			log_file_flag;
	BString			log_path;
	// advanced tab
	int32			max_connections;
		
	bool			is_zoomed;
	float			last_width;
	float			last_height;
	BRect			frame;
	BRect			setwindow_frame;
	
	
	// File Panels
	BFilePanel	*	saveConsoleFilePanel;
	BFilePanel	*	saveConsoleSelectionFilePanel;

};

#endif
