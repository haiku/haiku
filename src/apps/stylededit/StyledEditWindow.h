
#ifndef STYLED_EDIT_WINDOW_H
#define STYLED_EDIT_WINDOW_H

#ifndef _FILE_PANEL_H
#include <FilePanel.h>
#endif

#ifndef _MENUBAR_H
#include <MenuBar.h>
#endif

#ifndef _MESSAGE_H
#include <Message.h>
#endif

#ifndef _RECT_H
#include <Rect.h>
#endif

#ifndef _STRING_H
#include <String.h>
#endif

#ifndef _TEXTVIEW_H
#include <TextView.h>
#endif

#ifndef _WINDOW_H
#include <Window.h>
#endif

#ifndef STYLED_EDIT_VIEW_H
#include "StyledEditView.h"
#endif


class StyledEditWindow: public BWindow{
	public:
						StyledEditWindow(BRect frame);
						StyledEditWindow(BRect frame, entry_ref *ref);
						~StyledEditWindow();
		
					
		virtual bool 	QuitRequested();
		virtual void 	MessageReceived(BMessage *message);
		
		status_t 		Save(BMessage *message);
		void			OpenFile(entry_ref *ref); 
		status_t		PageSetup(const char *documentname);
		void			Print(const char *documentname);
		void			SearchAllWindows(BString find, BString replace, bool casesens);
	
	private: 
		void			InitWindow();
		void			Register(bool need_id);
		void			Unregister(void);
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
		BMenuItem		*fUndoItem;
		BMenuItem		*fCutItem;
		BMenuItem		*fCopyItem;
		BMenuItem		*fClearItem;
		BMenuItem		*fWrapItem;
		BString         fStringToFind;
		BString			fReplaceString;
		bool			fEnableItems;
		bool			fFirstEdit;
		bool			fTextSaved;
		bool			fUnDoneFlag;
		bool			fCaseSens;
		bool			fWrapAround;
		bool			fBackSearch;
		StyledEditView  *fTextView;
		BScrollView		*fScrollView;
		BFilePanel		*fSavePanel;
		int32			fWindow_Id;
		
};
#endif







					 