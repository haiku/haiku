/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//--------------------------------------------------------------------
//	
//	Mail.h
//
//--------------------------------------------------------------------

#ifndef _MAIL_H
#define _MAIL_H

#include <Application.h>
#include <Font.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MessageFilter.h>
#include <Point.h>
#include <PopUpMenu.h>
#include <Rect.h>
#include <Window.h>
#include <Entry.h>

#include <mail_encoding.h>

#define MAX_DICTIONARIES	8
#define	TITLE_BAR_HEIGHT	25
#define	WIND_WIDTH			457
#define WIND_HEIGHT			400
#define RIGHT_BOUNDARY 		8191
#define SEPARATOR_MARGIN	7
#define FONT_SIZE			11.0
#define QUOTE				"> "

enum MESSAGES {
	REFS_RECEIVED = 64,
	LIST_INVOKED,
	WINDOW_CLOSED,
	CHANGE_FONT,
	RESET_BUTTONS,
	PREFS_CHANGED,
	CHARSET_CHOICE_MADE
};

enum TEXT {
	SUBJECT_FIELD = REFS_RECEIVED + 64,
	TO_FIELD,
	ENCLOSE_FIELD,
	CC_FIELD,
	BCC_FIELD,
	NAME_FIELD
};

enum MENUS {
	/* app */
	M_NEW = SUBJECT_FIELD + 64,
	M_PREFS,
	M_EDIT_SIGNATURE,
	M_FONT,
	M_STYLE,
	M_SIZE,
	M_BEGINNER,
	M_EXPERT,
	/* file */
	M_REPLY,
	M_REPLY_TO_SENDER,
	M_REPLY_ALL,
	M_FORWARD,
	M_FORWARD_WITHOUT_ATTACHMENTS,
	M_RESEND,
	M_COPY_TO_NEW,
	M_HEADER,
	M_RAW,
	M_SEND_NOW,
	M_SAVE_AS_DRAFT,
	M_SAVE,
	M_PRINT_SETUP,
	M_PRINT,
	M_DELETE,
	M_DELETE_PREV,
	M_DELETE_NEXT,
	M_CLOSE_READ,
	M_CLOSE_SAVED,
	M_CLOSE_SAME,
	M_CLOSE_CUSTOM,
	M_STATUS,
	M_OPEN_MAIL_BOX,
	M_OPEN_MAIL_FOLDER,
	/* edit */
	M_SELECT,
	M_QUOTE,
	M_REMOVE_QUOTE,
	M_CHECK_SPELLING,
	M_SIGNATURE,
	M_RANDOM_SIG,
	M_SIG_MENU,
	M_FIND,
	M_FIND_AGAIN,
	/* encls */
	M_ADD,
	M_REMOVE,
	M_OPEN,
	M_COPY,
	/* nav */
	M_NEXTMSG,
	M_PREVMSG,
	M_SAVE_POSITION,
	/* Spam GUI button and menu items.  Order is important. */
	M_SPAM_BUTTON,
	M_TRAIN_SPAM_AND_DELETE,
	M_TRAIN_SPAM,
	M_UNTRAIN,
	M_TRAIN_GENUINE,

	M_REDO
};

enum USER_LEVEL {
	L_BEGINNER = 0,
	L_EXPERT
};

enum WINDOW_TYPES {
	MAIL_WINDOW = 0,
	PREFS_WINDOW,
	SIG_WINDOW
};

class TMailWindow;
class THeaderView;
class TEnclosuresView;
class TContentView;
class TMenu;
class TPrefsWindow;
class TSignatureWindow;
class BMenuItem;
class BmapButton;

class BFile;
class BFilePanel;
class ButtonBar;
class BMenuBar;
class Words;
class BEmailMessage;

//====================================================================

class TMailApp : public BApplication {
	public:
		TMailApp();
		~TMailApp();

		virtual void	AboutRequested();
		virtual void	ArgvReceived(int32, char **);
		virtual void	MessageReceived(BMessage *);
		virtual bool	QuitRequested();
		virtual void	ReadyToRun();
		virtual void	RefsReceived(BMessage *);

		TMailWindow		*FindWindow(const entry_ref &);
		void			FontChange();
		TMailWindow		*NewWindow(const entry_ref *rec = NULL, const char *to = NULL,
							bool resend = false, BMessenger *messenger = NULL);

		BFont fFont;

	private:
		void ClearPrintSettings();
		void CheckForSpamFilterExistence();

		status_t GetSettingsPath(BPath &path);
		status_t LoadOldSettings();
		status_t SaveSettings();
		status_t LoadSettings();

		BList			fWindowList;
		int32			fWindowCount;
		TPrefsWindow	*fPrefsWindow;
		TSignatureWindow *fSigWindow;

		uint8			fPreviousShowButtonBar;
};

//--------------------------------------------------------------------

class BMailMessage;

class TMailWindow : public BWindow {
	public:
		TMailWindow(BRect, const char *, const entry_ref *, const char *,
			const BFont *font, bool, BMessenger *trackerMessenger);
		virtual ~TMailWindow();

		virtual void FrameResized(float width, float height);
		virtual void MenusBeginning();
		virtual void MessageReceived(BMessage*);
		virtual bool QuitRequested();
		virtual void Show();
		virtual void Zoom(BPoint, float, float);
		virtual	void WindowActivated(bool state);

		void SetTo(const char *mailTo, const char *subject, const char *ccTo = NULL,
			const char *bccTo = NULL, const BString *body = NULL, BMessage *enclosures = NULL);
		void AddSignature(BMailMessage *);
		void Forward(entry_ref *, TMailWindow *, bool includeAttachments);
		void Print();
		void PrintSetup();
		void Reply(entry_ref *, TMailWindow *, uint32);
		void CopyMessage(entry_ref *ref, TMailWindow *src);
		status_t Send(bool);
		status_t SaveAsDraft( void );
		status_t OpenMessage(entry_ref *ref, uint32 characterSetForDecoding = B_MAIL_NULL_CONVERSION);

		status_t GetMailNodeRef(node_ref &nodeRef) const;
		BEmailMessage *Mail() const { return fMail; }

		bool GetTrackerWindowFile(entry_ref *, bool dir) const;
		void SaveTrackerPosition(entry_ref *);
		void SetOriginatingWindow(BWindow *window);

		void SetCurrentMessageRead();
		void SetTrackerSelectionToCurrent();
		TMailWindow* FrontmostWindow();
		void UpdateViews();

	protected:
		void SetTitleForMessage();
		void AddEnclosure(BMessage *msg);
		void BuildButtonBar();
		status_t TrainMessageAs (const char *CommandWord);

	private:
		BEmailMessage	*fMail;
		entry_ref *fRef;			// Reference to currently displayed file
		int32 fFieldState;
		BFilePanel *fPanel;
		BMenuBar *fMenuBar;
		BMenuItem *fAdd;
		BMenuItem *fCut;
		BMenuItem *fCopy;
		BMenuItem *fHeader;
		BMenuItem *fPaste;
		BMenuItem *fPrint;
		BMenuItem *fPrintSetup;
		BMenuItem *fQuote;
		BMenuItem *fRaw;
		BMenuItem *fRemove;
		BMenuItem *fRemoveQuote;
		BMenuItem *fSendNow;
		BMenuItem *fSendLater;
		BMenuItem *fUndo;
		BMenuItem *fRedo;
		BMenuItem *fNextMsg;
		BMenuItem *fPrevMsg;
		BMenuItem *fDeleteNext;
		BMenuItem *fSpelling;
		BMenu *fSaveAddrMenu;
		ButtonBar *fButtonBar;
		BmapButton *fSendButton;
		BmapButton *fSaveButton;
		BmapButton *fPrintButton;
		BmapButton *fSigButton;
		BRect fZoom;
		TContentView *fContentView;
		THeaderView *fHeaderView;
		TEnclosuresView *fEnclosuresView;
		TMenu *fSignature;
		BMessenger fTrackerMessenger;
			// Talks to tracker window that this was launched from.

		entry_ref fPrevRef, fNextRef;
		bool fPrevTrackerPositionSaved : 1;
		bool fNextTrackerPositionSaved : 1;

		static BList sWindowList;
		static BLocker sWindowListLock;

		bool fSigAdded;
		bool fIncoming;
		bool fReplying;
		bool fResending;
		bool fSent;
		bool fDraft;
		bool fChanged;
	
		char *fStartingText;	
		entry_ref	fRepliedMail;
		BMessenger	*fOriginatingWindow;
};

//====================================================================

class TMenu: public BPopUpMenu
{
	public:
		TMenu(const char *, const char *, int32, bool popup = false, bool addRandom = true);
		~TMenu();
						
		virtual BPoint	ScreenLocation(void);
		virtual void	AttachedToWindow();

		void			BuildMenu();
	
	private:
		char	*fAttribute;
		char	*fPredicate;
		bool	fPopup, fAddRandom;
		int32	fMessage;
};

//====================================================================

int32 header_len(BFile *);
extern Words *gWords[MAX_DICTIONARIES];
extern Words *gExactWords[MAX_DICTIONARIES];
extern int32 gUserDict;
extern BFile *gUserDictFile;
extern int32 gDictCount;

#endif // #ifndef _MAIL_H
