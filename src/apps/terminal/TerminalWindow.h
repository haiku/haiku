#ifndef TERMINAL_WINDOW_H
#define TERMINAL_WINDOW_H

#include <FilePanel.h>
#include <MenuBar.h>
#include <Message.h>
#include <Rect.h>
#include <String.h>
#include <TextView.h>
#include <Window.h>
#include <Message.h>

class TerminalTextView;

class TerminalWindow
	: public BWindow
{
public:
	                  TerminalWindow(BMessage * settings = 0);
	virtual           ~TerminalWindow();
	
	virtual void      Quit(void);
	virtual bool      QuitRequested(void);
	virtual void      MessageReceived(BMessage *message);
	virtual void      MenusBeginning(void);
	virtual status_t  InitCheck(void);
	
private:
	status_t          InitWindow(int32 id, entry_ref * settingsRef = 0);
	status_t          RestoreSettings(entry_ref * settingsRef = 0);

	// message received helpers
	void              StartNewTerminal(BMessage * message);
	void              SwitchTerminals(BMessage * message);
	void              EnableEditItems(BMessage * message);
	void              DisableEditItems(BMessage * message);
	void              EditCopy(BMessage * message);
	void              EditPaste(BMessage * message);
	void              EditClearAll(BMessage * message);
	
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
	BView           *fShellView;
	BScrollView     *fScrollView;
	
	// File panels
	BFilePanel		*fLogToFilePanel;
	BMenu			*fLogToFilePanelEncodingMenu;
	BFilePanel		*fWriteSelectionPanel;
	BMenu			*fWriteSelectionPanelEncodingMenu;
	BFilePanel		*fSaveAsSettingsFilePanel;
	BMenu			*fSaveAsSettingsFilePanelEncodingMenu;

	BTextControl	*fSavePanelTextView;
	
	status_t        fInitStatus;
};

#endif // TERMINAL_WINDOW_H
