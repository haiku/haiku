/* PoorManWindow.h
 *
 *	Philip Harrison
 *	Started: 4/25/2004
 *	Version: 0.1
 */

#ifndef POOR_MAN_WINDOW_H
#define POOR_MAN_WINDOW_H

#include <pthread.h>

#include <SupportDefs.h>
#include <Window.h>
#include <String.h>

class BPoint;
class BFilePanel;
class BMessage;
class BMenuBar;
class BMenu;
class BTextView;
class BStringView;
class BScrollView;
class BRect;
class BFile;
class BFont;

class PoorManView;
class PoorManPreferencesWindow;
class PoorManServer;

class PoorManWindow: public BWindow
{
public:
					PoorManWindow(BRect frame);
virtual				~PoorManWindow();
virtual	void		MessageReceived(BMessage * message);

virtual void		FrameMoved(BPoint origin);
virtual	void		FrameResized(float width, float height);
virtual	bool		QuitRequested();
virtual	void		Zoom(BPoint origin, float width, float height);

	// -------------------------------------------
	// Public PoorMan Window Methods
		void		SetDirLabel(const char * name);
		void		SetHits(uint32 num);
		uint32		GetHits() { return hits; }
		status_t	SaveConsole(BMessage * message, bool);
		
		status_t	SaveSettings();
		status_t	ReadSettings();
		void		DefaultSettings();
		
		status_t	StartServer();
		status_t	StopServer();
		
		PoorManServer* GetServer()const{return fServer;}
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
		void		 SetLogPath(const char * str);
		// Advanced Tab
		int16		 MaxConnections()			  { return max_connections; }
		void		 SetMaxConnections(int16 num) { max_connections = num;  }


private:
	// -------------------------------------------
	// PoorMan Window Methods
		void		UpdateStatusLabelAndMenuItem();
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
	int16			max_connections;
		
	bool			is_zoomed;
	float			last_width;
	float			last_height;
	BRect			frame;
	BRect			setwindow_frame;
	
	// File Panels
	BFilePanel	*	saveConsoleFilePanel;
	BFilePanel	*	saveConsoleSelectionFilePanel;

	BFile* fLogFile;
	BFont* fLogViewFont;
	
	PoorManServer* fServer;
	
	pthread_rwlock_t fLogFileLock;
};

#endif
