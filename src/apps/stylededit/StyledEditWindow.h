#ifndef STYLED_EDIT_WINDOW_H
#define STYLED_EDIT_WINDOW_H

#include <FilePanel.h>
#include <MenuBar.h>
#include <Message.h>
#include <Rect.h>
#include <String.h>
#include <TextView.h>
#include <Window.h>

class StyledEditView;

class StyledEditWindow
	: public BWindow
{
public:
					StyledEditWindow(BRect frame, int32 id);
					StyledEditWindow(BRect frame, entry_ref *ref);
					~StyledEditWindow();
	
	virtual void	Quit();
	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage *message);
	
	status_t 		Save(BMessage *message);
	void			OpenFile(entry_ref *ref); 
	status_t		PageSetup(const char *documentname);
	void			Print(const char *documentname);
	void			SearchAllWindows(BString find, BString replace, bool casesens);

private: 
	void			InitWindow();
	bool			Search(BString searchfor, bool casesens, bool wrap, bool backsearch);
	void			FindSelection();
	void			Replace(BString findthis, BString replacewith, bool casesens, bool wrap, bool backsearch);
	void			ReplaceAll(BString find, BString replace, bool casesens);
	void			SetFontSize(float fontSize);
	void			SetFontColor(rgb_color *color);
	void			SetFontStyle(const char *fontFamily, const char *fontStyle);
	void			RevertToSaved();
	
	BMenuBar		*fMenuBar;
	BMessage		*fPrintSettings;
	BMessage		*fSaveMessage; 
	BMenuItem		*fSaveItem;
	BMenuItem		*fRevertItem;
	BMenuItem		*fUndoItem;
	BMenuItem		*fCutItem;
	BMenuItem		*fCopyItem;
	BMenuItem		*fClearItem;
	BMenuItem		*fWrapItem;
	BString         fStringToFind;
	BString			fReplaceString;
	
	// undo modes
	bool 			fUndoFlag;   // we just did an undo action
	bool			fCanUndo;    // we can do an undo action next
	bool 			fRedoFlag;   // we just did a redo action
	bool			fCanRedo;    // we can do a redo action next
	
	// clean modes
	bool			fUndoCleans; // an undo action will put us in a clean state
	bool			fRedoCleans; // a redo action will put us in a clean state
	bool			fClean;      // we are in a clean state
	
	bool			fCaseSens;
	bool			fWrapAround;
	bool			fBackSearch;
	
	StyledEditView  *fTextView;
	BScrollView		*fScrollView;
	BFilePanel		*fSavePanel;
		
};

#endif // STYLED_EDIT_WINDOW_H
