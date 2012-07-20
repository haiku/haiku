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
	virtual					~PoorManWindow();
	virtual	void			MessageReceived(BMessage* message);

	virtual void			FrameMoved(BPoint origin);
	virtual	void			FrameResized(float width, float height);
	virtual	bool			QuitRequested();
	virtual	void			Zoom(BPoint origin, float width, float height);

	// -------------------------------------------
	// Public PoorMan Window Methods
			void			SetDirLabel(const char* name);
			void			SetHits(uint32 num);
			uint32			GetHits() { return fHits; }
			status_t		SaveConsole(BMessage* message, bool);
		
			status_t		SaveSettings();
			status_t		ReadSettings();
			void			DefaultSettings();
		
			status_t		StartServer();
			status_t		StopServer();
		
			PoorManServer* 	GetServer() const { return fServer;}
	// -------------------------------------------	
	// Preferences and Settings
		// Site Tab		
	bool DirListFlag()
	{
		return fDirListFlag;
	}

	void SetDirListFlag(bool flag)
	{
		fDirListFlag = flag;
	}

	const char* IndexFileName()
	{
		return fIndexFileName.String();
	}

	void SetIndexFileName(const char* str)
	{
		fIndexFileName.SetTo(str);
	}

	const char*	WebDir()
	{
		return fWebDirectory.String();
	}

	void SetWebDir(const char* str)
	{
		fWebDirectory.SetTo(str);
	}

	// Logging Tab

	bool LogConsoleFlag()
	{
		return fLogConsoleFlag;
	}

	void SetLogConsoleFlag(bool flag)
	{
 		fLogConsoleFlag = flag;
	}

	bool LogFileFlag()
	{
		return fLogFileFlag;
	}

	void SetLogFileFlag(bool flag)
	{
		fLogFileFlag = flag;
	}

	const char* LogPath()
	{
		return fLogPath.String();
	}

			void SetLogPath(const char* str);

	// Advanced Tab
	int16 MaxConnections()
	{
 		return fMaxConnections;
	}

	void SetMaxConnections(int16 num)
	{
		fMaxConnections = num;
	}

private:
	// -------------------------------------------
	// PoorMan Window Methods
			void			UpdateStatusLabelAndMenuItem();
			void			UpdateHitsLabel();
	
private:
	// -------------------------------------------
	// Build Menu Methods
			BMenu*			BuildFileMenu() const;
			BMenu*			BuildEditMenu() const;
			BMenu*			BuildControlsMenu() const;
	
	// --------------------------------------------
	// MenuBar & Menu items
			BMenuBar*		fFileMenuBar;
			BMenu*			fFileMenu;
			BMenu*			fEditMenu;
			BMenu*			fControlsMenu;
	
	// --------------------------------------------
	// Status, Hits, Directory
			BStringView*	fStatusView;
			BStringView*	fHitsView;
			BStringView*	fDirView;
	
			bool			fStatus;
			uint32			fHits;
			char			fHitsLabel[25];
	
	// --------------------------------------------
	// Logging View
			BScrollView*	fScrollView;
			BTextView*		fLoggingView;
	// use asctime() for format of [Date/Time]:
	

	// -------------------------------------------
	// PoorMan Preference Window
			PoorManPreferencesWindow * fPrefWindow;
	
	// site tab
			BString			fWebDirectory;
			BString			fIndexFileName;
			bool			fDirListFlag;

	// logging tab
			bool			fLogConsoleFlag;
			bool			fLogFileFlag;
			BString			fLogPath;
	
	// advanced tab
			int16			fMaxConnections;
		
			bool			fIsZoomed;
			float			fLastWidth;
			float			fLastHeight;
			BRect			fFrame;
			BRect			fSetwindowFrame;
	
	// File Panels
			BFilePanel*		fSaveConsoleFilePanel;
			BFilePanel*		fSaveConsoleSelectionFilePanel;

			BFile* 			fLogFile;
	
			PoorManServer*	fServer;
	
			pthread_rwlock_t fLogFileLock;
};

#endif
