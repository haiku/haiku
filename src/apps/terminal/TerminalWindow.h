#ifndef TERMINAL_WINDOW_H
#define TERMINAL_WINDOW_H

#include <FilePanel.h>
#include <MenuBar.h>
#include <Message.h>
#include <Rect.h>
#include <String.h>
#include <TextView.h>
#include <Window.h>

class ToggleScrollView;
class TerminalTextView;

class TerminalWindow
	: public BWindow
{
public:
					TerminalWindow(BPoint topLeft, int32 id);
					TerminalWindow(entry_ref * ref, int32 id);
					~TerminalWindow();
	
	virtual void	Quit();
	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage *message);
	virtual void	MenusBeginning();
	
private: 
	status_t		InitWindow(int32 id, entry_ref * settingsRef = 0);
	status_t		RestoreSettings(entry_ref * settingsRef = 0);

	// Menu variables
	BMenuBar		*fMenuBar;
	
	BMenu			*fTerminal;
	// ----------------------------------
	BMenuItem		*fSwitchTerminals;
	BMenuItem		*fStartNewTerminal;
	BMenuItem		*fLogToFile;
	
	BMenu			*fEdit;
	// ----------------------------------
	BMenuItem		*fCopy;
	BMenuItem		*fPaste;
	// ----------------------------------
	BMenuItem		*fSelectAll;
	BMenuItem		*fWriteSelection;
	BMenuItem		*fClearAll;
	// ----------------------------------
	BMenuItem		*fFind;
	BMenuItem		*fFindBackward;
	BMenuItem		*fFindForward;	
	
	BMenu			*fSettings;
	// ----------------------------------
	BMenu			*fWindowSize;
	BMenu			*fFont;
	BMenuItem		*fFontSize;
	BMenu			*fFontEncoding;
	BMenuItem		*fTabWidth;
	BMenuItem		*fColor;
	// ----------------------------------
	BMenuItem		*fSaveAsDefault;
	BMenuItem		*fSaveAsSettingsFile;
	
	// Main views
	TerminalTextView*fTextView;
	ToggleScrollView*fScrollView;
	
	// File panels
	BFilePanel		*fLogToFilePanel;
	BMenu			*fLogToFilePanelEncodingMenu;
	BFilePanel		*fWriteSelectionPanel;
	BMenu			*fWriteSelectionPanelEncodingMenu;
	BFilePanel		*fSaveAsSettingsFilePanel;
	BMenu			*fSaveAsSettingsFilePanelEncodingMenu;

	BTextControl	*fSavePanelTextView;
		
};

#endif // TERMINAL_WINDOW_H
