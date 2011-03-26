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
#ifndef _MAIL_APP_H
#define _MAIL_APP_H

#include <Application.h>
#include <Catalog.h>
#include <Entry.h>
#include <Font.h>
#include <List.h>
#include <String.h>


class BFile;
class BMessenger;
class TMailWindow;
class TPrefsWindow;
class TSignatureWindow;


class TMailApp : public BApplication {
	public:
								TMailApp();
		virtual					~TMailApp();

		virtual	void			ArgvReceived(int32, char**);
		virtual	void			MessageReceived(BMessage*);
		virtual	bool			QuitRequested();
		virtual	void			ReadyToRun();
		virtual	void			RefsReceived(BMessage*);

				TMailWindow*	FindWindow(const entry_ref&);
				void			FontChange();
				TMailWindow*	NewWindow(const entry_ref* rec = NULL,
									const char* to = NULL, bool resend = false,
									BMessenger* messenger = NULL);

				void			SetPrintSettings(const BMessage* settings);
				bool			HasPrintSettings();
				BMessage		PrintSettings();

				void			SetLastWindowFrame(BRect frame);

				// TODO: move these into a MailSettings class
				bool			AutoMarkRead();
				BString			Signature();
				BString			ReplyPreamble();
				bool			WrapMode();
				bool			AttachAttributes();
				bool			ColoredQuotes();
				uint8			ShowButtonBar();
				bool			WarnAboutUnencodableCharacters();
				bool			StartWithSpellCheckOn();
				void			SetDefaultAccount(int32 account);
				int32			DefaultAccount();
				int32			UseAccountFrom();
				uint32			MailCharacterSet();
				bool			ShowSpamGUI() const
									{ return fShowSpamGUI; }
				BFont			ContentFont();

	private:
				void			_ClearPrintSettings();
				void			_CheckForSpamFilterExistence();

				status_t		GetSettingsPath(BPath &path);
				status_t		LoadOldSettings();
				status_t		SaveSettings();
				status_t		LoadSettings();

				BList			fWindowList;
				int32			fWindowCount;
				TPrefsWindow*	fPrefsWindow;
				TSignatureWindow* fSigWindow;
		
				BRect			fMailWindowFrame;
				BRect			fLastMailWindowFrame;
				BRect			fSignatureWindowFrame;
				BPoint			fPrefsWindowPos;

				BMessage*		fPrintSettings;

				bool			fPrintHelpAndExit;

				// TODO: these should go into a settings class
				bool			fAutoMarkRead;
				char*			fSignature;
				char*			fReplyPreamble;
				bool			fWrapMode;
				bool			fAttachAttributes;
				bool			fColoredQuotes;
				uint8			fShowButtonBar;
				bool			fWarnAboutUnencodableCharacters;
				bool			fStartWithSpellCheckOn;
				bool			fShowSpamGUI;
				int32			fDefaultAccount;
				int32			fUseAccountFrom;
				uint32			fMailCharacterSet;
				BFont			fContentFont;
};


#endif // #ifndef _MAIL_H

