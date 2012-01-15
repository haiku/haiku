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
#ifndef _CONTENT_H
#define _CONTENT_H

#include <FilePanel.h>
#include <FindDirectory.h>
#include <Font.h>
#include <fs_attr.h>
#include <Point.h>
#include <Rect.h>
#include <MessageFilter.h>

#include "KUndoBuffer.h"

#define MESSAGE_TEXT		"Message:"
#define MESSAGE_TEXT_H		 16
#define MESSAGE_TEXT_V		 5
#define MESSAGE_FIELD_H		 59
#define MESSAGE_FIELD_V		 11

#define CONTENT_TYPE		"content-type: "
#define CONTENT_ENCODING	"content-transfer-encoding: "
#define CONTENT_DISPOSITION	"Content-Disposition: "
#define MIME_TEXT			"text/"
#define MIME_MULTIPART		"multipart/"

class TMailWindow;
class TScrollView;
class TTextView;
class BFile;
class BList;
class BPopupMenu;

struct text_run_array;

typedef struct {
	bool header;
	bool raw;
	bool quote;
	bool incoming;
	bool close;
	bool mime;
	TTextView *view;
	BEmailMessage *mail;
	BList *enclosures;
	sem_id *stop_sem;
} reader_info;

enum ENCLOSURE_TYPE {
	TYPE_ENCLOSURE = 100,
	TYPE_BE_ENCLOSURE,
	TYPE_URL,
	TYPE_MAILTO
};

struct hyper_text {
	int32 type;
	char *name;
	char *content_type;
	char *encoding;
	int32 text_start;
	int32 text_end;
	BMailComponent *component;
	bool saved;
	bool have_ref;
	entry_ref ref;
	node_ref node;
};

class TSavePanel;


//====================================================================

class TContentView : public BView {
	public:
		TContentView(BRect, bool incoming, BFont*,
			bool showHeader, bool coloredQuotes); 
		virtual void MessageReceived(BMessage *);
		void FindString(const char *);
		void Focus(bool);
		void FrameResized(float, float);

		TTextView *fTextView;

	private:
		bool	fFocus;
		bool	fIncoming;
		float	fOffset;
};

//====================================================================

enum {
	S_CLEAR_ERRORS = 1,
	S_SHOW_ERRORS = 2
};

struct quote_context {
	quote_context()
		:
		level(0),
		diff_mode(0),
		begin(true),
		in_diff(false),
		was_diff(false)
	{
	}

	int32	level;
	int32	diff_mode;
	bool	begin;
	bool	in_diff;
	bool	was_diff;
};

class TTextView : public BTextView {
	public:
								TTextView(BRect, BRect, bool incoming,
									TContentView*, BFont*, bool showHeader,
									bool coloredQuotes);
								~TTextView();

		virtual	void AttachedToWindow();
		virtual void KeyDown(const char*, int32);
		virtual void MakeFocus(bool);
		virtual void MessageReceived(BMessage*);
		virtual void MouseDown(BPoint);
		virtual void MouseMoved(BPoint, uint32, const BMessage*);
		virtual void InsertText(const char *text, int32 length, int32 offset,
			const text_run_array *runs);
		virtual void  DeleteText(int32 start, int32 finish);

		void ClearList();
		void LoadMessage(BEmailMessage *mail, bool quoteIt, const char *insertText);
		void Open(hyper_text*);
		status_t Save(BMessage *, bool makeNewFile = true);
		void StopLoad();
		bool IsReaderThreadRunning();
		void AddAsContent(BEmailMessage *mail, bool wrap, uint32 charset, mail_encoding encoding);
		void CheckSpelling(int32 start, int32 end,
			int32 flags = S_CLEAR_ERRORS | S_SHOW_ERRORS);
		void FindSpellBoundry(int32 length, int32 offset, int32 *start,
			int32 *end);
		void EnableSpellCheck(bool enable);

		void AddQuote(int32 start, int32 finish);
		void RemoveQuote(int32 start, int32 finish);
		void UpdateFont(const BFont* newFont);

		void	WindowActivated(bool flag);
		void	Undo(BClipboard *clipboard);
		void	Redo();

		const BFont *Font() const { return &fFont; }

		bool fHeader;
		bool fColoredQuotes;
		bool fReady;

	private:
		struct { bool replaced, deleted; } fUndoState;
		KUndoBuffer	fUndoBuffer;

		struct { bool active, replace; } fInputMethodUndoState;
		KUndoBuffer	fInputMethodUndoBuffer;
			// For handling Input Method changes in undo.

		struct spell_mark;

		spell_mark *FindSpellMark(int32 start, int32 end, spell_mark **_previousMark = NULL);
		void UpdateSpellMarks(int32 offset, int32 length);
		status_t AddSpellMark(int32 start, int32 end);
		bool RemoveSpellMark(int32 start, int32 end);
		void RemoveSpellMarks();

		void ContentChanged(void);

		class Reader;
		friend class TTextView::Reader;

		char *fYankBuffer;
		int32 fLastPosition;
		BFile *fFile;
		BEmailMessage *fMail;
			// for incoming/replied/forwarded mails only
		BFont fFont;
		TContentView *fParent;
		sem_id fStopSem;
		bool fStopLoading;
		thread_id fThread;
		BList *fEnclosures;
		BPopUpMenu *fEnclosureMenu;
		BPopUpMenu *fLinkMenu;
		TSavePanel *fPanel;
		bool fIncoming;
		bool fSpellCheck;
		bool fRaw;
		bool fCursor;

		struct spell_mark {
			spell_mark *next;
			int32	start;
			int32	end;
			struct text_run_array *style;
		};

		spell_mark *fFirstSpellMark;

		class Reader {
			public:
				Reader(bool header, bool raw, bool quote, bool incoming,
					bool stripHeaders, bool mime, TTextView* view,
					BEmailMessage* mail, BList* list, sem_id sem);

				static status_t Run(void* _dummy);

			private:
				bool ParseMail(BMailContainer* container,
					BTextMailComponent* ignore);
				bool Process(const char* data, int32 length,
					bool isHeader = false);
				bool Insert(const char* line, int32 count, bool isHyperLink,
					bool isHeader = false);

				bool Lock();
				status_t Unlock();

				bool fHeader;
				bool fRaw;
				bool fQuote;
				quote_context fQuoteContext;
				bool fIncoming;
				bool fStripHeader;
				bool fMime;
				TTextView* fView;
				BEmailMessage* fMail;
				BList *fEnclosures;
				sem_id fStopSem;
		};
};


//====================================================================

class TSavePanel : public BFilePanel {
	public:
		TSavePanel(hyper_text*, TTextView*);
		virtual void SendMessage(const BMessenger*, BMessage*);
		void SetEnclosure(hyper_text*);
	
	private:
		hyper_text *fEnclosure;
		TTextView *fView;
};

//====================================================================

class TextRunArray {
	public:
		TextRunArray(size_t entries);
		~TextRunArray();

		text_run_array &Array() { return *fArray; }
		size_t MaxEntries() const { return fNumEntries; }

	private:
		text_run_array *fArray;
		size_t fNumEntries;
};

extern void FillInQuoteTextRuns(BTextView* view, quote_context* context,
	const char* line, int32 length, const BFont& font, text_run_array* style,
	int32 maxStyles = 5);

#endif	/* #ifndef _CONTENT_H */

