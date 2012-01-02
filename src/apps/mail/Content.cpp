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
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or
registered trademarks of Be Incorporated in the United States and other
countries. Other brand product names are registered trademarks or trademarks
of their respective holders. All rights reserved.
*/


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include <Alert.h>
#include <Beep.h>
#include <Clipboard.h>
#include <Debug.h>
#include <E-mail.h>
#include <Input.h>
#include <Locale.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Roster.h>
#include <ScrollView.h>
#include <TextView.h>
#include <UTF8.h>

#include <MailMessage.h>
#include <MailAttachment.h>
#include <mail_util.h>

#include "MailApp.h"
#include "MailSupport.h"
#include "MailWindow.h"
#include "Messages.h"
#include "Content.h"
#include "Utilities.h"
#include "FieldMsg.h"
#include "Words.h"


#define DEBUG_SPELLCHECK 0
#if DEBUG_SPELLCHECK
#	define DSPELL(x) x
#else
#	define DSPELL(x) ;
#endif


#define B_TRANSLATE_CONTEXT "Mail"


const rgb_color kNormalTextColor = {0, 0, 0, 255};
const rgb_color kSpellTextColor = {255, 0, 0, 255};
const rgb_color kHyperLinkColor = {0, 0, 255, 255};
const rgb_color kHeaderColor = {72, 72, 72, 255};

const rgb_color kQuoteColors[] = {
	{0, 0, 0x80, 0},		// 3rd, 6th, ... quote level color (blue)
	{0, 0x80, 0, 0},		// 1st, 4th, ... quote level color (green)
	{0x80, 0, 0, 0}			// 2nd, ... (red)
};
const int32 kNumQuoteColors = 3;

const rgb_color kDiffColors[] = {
	{0xb0, 0, 0, 0},		// '-', red
	{0, 0x90, 0, 0},		// '+', green
	{0x6a, 0x6a, 0x6a, 0}	// '@@', dark grey
};

void Unicode2UTF8(int32 c, char **out);


inline bool
IsInitialUTF8Byte(uchar b)
{
	return ((b & 0xC0) != 0x80);
}


void
Unicode2UTF8(int32 c, char **out)
{
	char *s = *out;

	ASSERT(c < 0x200000);

	if (c < 0x80)
		*(s++) = c;
	else if (c < 0x800)
	{
		*(s++) = 0xc0 | (c >> 6);
		*(s++) = 0x80 | (c & 0x3f);
	}
	else if (c < 0x10000)
	{
		*(s++) = 0xe0 | (c >> 12);
		*(s++) = 0x80 | ((c >> 6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
	}
	else if (c < 0x200000)
	{
		*(s++) = 0xf0 | (c >> 18);
		*(s++) = 0x80 | ((c >> 12) & 0x3f);
		*(s++) = 0x80 | ((c >> 6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
	}
	*out = s;
}


static bool
FilterHTMLTag(int32 &first, char **t, char *end)
{
	const char *newlineTags[] = {
		"br", "/p", "/div", "/table", "/tr",
		NULL};

	char *a = *t;

	// check for some common entities (in ISO-Latin-1)
	if (first == '&') {
		// filter out and convert decimal values
		if (a[1] == '#' && sscanf(a + 2, "%ld;", &first) == 1) {
			t[0] += strchr(a, ';') - a;
			return false;
		}

		const struct { const char *name; int32 code; } entities[] = {
			// this list is sorted alphabetically to be binary searchable
			// the current implementation doesn't do this, though

			// "name" is the entity name,
			// "code" is the corresponding unicode
			{"AElig;",	0x00c6},
			{"Aacute;",	0x00c1},
			{"Acirc;",	0x00c2},
			{"Agrave;",	0x00c0},
			{"Aring;",	0x00c5},
			{"Atilde;",	0x00c3},
			{"Auml;",	0x00c4},
			{"Ccedil;",	0x00c7},
			{"Eacute;",	0x00c9},
			{"Ecirc;",	0x00ca},
			{"Egrave;",	0x00c8},
			{"Euml;",	0x00cb},
			{"Iacute;", 0x00cd},
			{"Icirc;",	0x00ce},
			{"Igrave;", 0x00cc},
			{"Iuml;",	0x00cf},
			{"Ntilde;",	0x00d1},
			{"Oacute;", 0x00d3},
			{"Ocirc;",	0x00d4},
			{"Ograve;", 0x00d2},
			{"Ouml;",	0x00d6},
			{"Uacute;", 0x00da},
			{"Ucirc;",	0x00db},
			{"Ugrave;", 0x00d9},
			{"Uuml;",	0x00dc},
			{"aacute;", 0x00e1},
			{"acirc;",	0x00e2},
			{"aelig;",	0x00e6},
			{"agrave;", 0x00e0},
			{"amp;",	'&'},
			{"aring;",	0x00e5},
			{"atilde;", 0x00e3},
			{"auml;",	0x00e4},
			{"ccedil;",	0x00e7},
			{"copy;",	0x00a9},
			{"eacute;",	0x00e9},
			{"ecirc;",	0x00ea},
			{"egrave;",	0x00e8},
			{"euml;",	0x00eb},
			{"gt;",		'>'},
			{"iacute;", 0x00ed},
			{"icirc;",	0x00ee},
			{"igrave;", 0x00ec},
			{"iuml;",	0x00ef},
			{"lt;",		'<'},
			{"nbsp;",	' '},
			{"ntilde;",	0x00f1},
			{"oacute;", 0x00f3},
			{"ocirc;",	0x00f4},
			{"ograve;", 0x00f2},
			{"ouml;",	0x00f6},
			{"quot;",	'"'},
			{"szlig;",	0x00f6},
			{"uacute;", 0x00fa},
			{"ucirc;",	0x00fb},
			{"ugrave;", 0x00f9},
			{"uuml;",	0x00fc},
			{NULL, 0}
		};

		for (int32 i = 0; entities[i].name; i++) {
			// entities are case-sensitive
			int32 length = strlen(entities[i].name);
			if (!strncmp(a + 1, entities[i].name, length)) {
				t[0] += length;	// note that the '&' is included here
				first = entities[i].code;
				return false;
			}
		}
	}

	// no tag to filter
	if (first != '<')
		return false;

	a++;

	// is the tag one of the newline tags?

	bool newline = false;
	for (int i = 0; newlineTags[i]; i++) {
		int length = strlen(newlineTags[i]);
		if (!strncasecmp(a, (char *)newlineTags[i], length) && !isalnum(a[length])) {
			newline = true;
			break;
		}
	}

	// oh, it's not, so skip it!

	if (!strncasecmp(a, "head", 4)) {	// skip "head" completely
		for (; a[0] && a < end; a++) {
			// Find the end of the HEAD section, or the start of the BODY,
			// which happens for some malformed spam.
			if (strncasecmp (a, "</head", 6) == 0 ||
				strncasecmp (a, "<body", 5) == 0)
				break;
		}
	}

	// skip until tag end
	while (a[0] && a[0] != '>' && a < end)
		a++;

	t[0] = a;

	if (newline) {
		first = '\n';
		return false;
	}

	return true;
}


/** Returns the type and length of the URL in the string if it is one.
 *	If the "url" string is specified, it will fill it with the complete
 *	URL that might differ from the one in the text itself (i.e. because
 *	of an prepended "http://").
 */

static uint8
CheckForURL(const char *string, size_t &urlLength, BString *url = NULL)
{
	const char *urlPrefixes[] = {
		"http://",
		"ftp://",
		"shttp://",
		"https://",
		"finger://",
		"telnet://",
		"gopher://",
		"news://",
		"nntp://",
		"file://",
		NULL
	};

	//
	//	Search for URL prefix
	//
	uint8 type = 0;
	for (const char **prefix = urlPrefixes; *prefix != 0; prefix++) {
		if (!cistrncmp(string, *prefix, strlen(*prefix))) {
			type = TYPE_URL;
			break;
		}
	}

	//
	//	Not a URL? check for "mailto:" or "www."
	//
	if (type == 0 && !cistrncmp(string, "mailto:", 7))
		type = TYPE_MAILTO;
	if (type == 0 && !strncmp(string, "www.", 4)) {
		// this type will be special cased later (and a http:// is added
		// for the enclosure address)
		type = TYPE_URL;
	}
	if (type == 0) {
		// check for valid eMail addresses
		const char *at = strchr(string, '@');
		if (at) {
			const char *pos = string;
			bool readName = false;
			for (; pos < at; pos++) {
				// ToDo: are these all allowed characters?
				if (!isalnum(pos[0]) && pos[0] != '_' && pos[0] != '.' && pos[0] != '-')
					break;

				readName = true;
			}
			if (pos == at && readName)
				type = TYPE_MAILTO;
		}
	}

	if (type == 0)
		return 0;

	int32 index = strcspn(string, " \t<>)\"\\,\r\n");

	// filter out some punctuation marks if they are the last character
	char suffix = string[index - 1];
	if (suffix == '.'
		|| suffix == ','
		|| suffix == '?'
		|| suffix == '!'
		|| suffix == ':'
		|| suffix == ';')
		index--;

	if (type == TYPE_URL) {
		char *parenthesis = NULL;

		// filter out a trailing ')' if there is no left parenthesis before
		if (string[index - 1] == ')') {
			char *parenthesis = strchr(string, '(');
			if (parenthesis == NULL || parenthesis > string + index)
				index--;
		}

		// filter out a trailing ']' if there is no left bracket before
		if (parenthesis == NULL && string[index - 1] == ']') {
			char *parenthesis = strchr(string, '[');
			if (parenthesis == NULL || parenthesis > string + index)
				index--;
		}
	}

	if (url != NULL) {
		// copy the address to the specified string
		if (type == TYPE_URL && string[0] == 'w') {
			// URL starts with "www.", so add the protocol to it
			url->SetTo("http://");
			url->Append(string, index);
		} else if (type == TYPE_MAILTO && cistrncmp(string, "mailto:", 7)) {
			// eMail address had no "mailto:" prefix
			url->SetTo("mailto:");
			url->Append(string, index);
		} else
			url->SetTo(string, index);
	}
	urlLength = index;

	return type;
}


static void
CopyQuotes(const char *text, size_t length, char *outText, size_t &outLength)
{
	// count qoute level (to be able to wrap quotes correctly)

	const char *quote = QUOTE;
	int32 level = 0;
	for (size_t i = 0; i < length; i++) {
		if (text[i] == quote[0])
			level++;
		else if (text[i] != ' ' && text[i] != '\t')
			break;
	}

	// if there are too much quotes, try to preserve the quote color level
	if (level > 10)
		level = kNumQuoteColors * 3 + (level % kNumQuoteColors);

	// copy the quotes to outText

	const int32 quoteLength = strlen(QUOTE);
	outLength = 0;
	while (level-- > 0) {
		strcpy(outText + outLength, QUOTE);
		outLength += quoteLength;
	}
}


int32
diff_mode(char c)
{
	if (c == '+')
		return 2;
	if (c == '-')
		return 1;
	if (c == '@')
		return 3;
	if (c == ' ')
		return 0;

	// everything else ends the diff mode
	return -1;
}


bool
is_quote_char(char c)
{
	return c == '>' || c == '|';
}


/*!	Fills the specified text_run_array with the correct values for the
	specified text.
	If "view" is NULL, it will assume that "line" lies on a line break,
	if not, it will correctly retrieve the number of quotes the current
	line already has.
*/
void
FillInQuoteTextRuns(BTextView* view, quote_context* context, const char* line,
	int32 length, const BFont& font, text_run_array* style, int32 maxStyles)
{
	text_run* runs = style->runs;
	int32 index = style->count;
	bool begin;
	int32 pos = 0;
	int32 diffMode = 0;
	bool inDiff = false;
	bool wasDiff = false;
	int32 level = 0;

	// get index to the beginning of the current line

	if (context != NULL) {
		level = context->level;
		diffMode = context->diff_mode;
		begin = context->begin;
		inDiff = context->in_diff;
		wasDiff = context->was_diff;
	} else if (view != NULL) {
		int32 start, end;
		view->GetSelection(&end, &end);

		begin = view->TextLength() == 0
			|| view->ByteAt(view->TextLength() - 1) == '\n';

		// the following line works only reliable when text wrapping is set to
		// off; so the complicated version actually used here is necessary:
		// start = view->OffsetAt(view->CurrentLine());

		const char *text = view->Text();

		if (!begin) {
			// if the text is not the start of a new line, go back
			// to the first character in the current line
			for (start = end; start > 0; start--) {
				if (text[start - 1] == '\n')
					break;
			}
		}

		// get number of nested qoutes for current line

		if (!begin && start < end) {
			begin = true;
				// if there was no text in this line, there may come
				// more nested quotes

			diffMode = diff_mode(text[start]);
			if (diffMode == 0) {
				for (int32 i = start; i < end; i++) {
					if (is_quote_char(text[i]))
						level++;
					else if (text[i] != ' ' && text[i] != '\t') {
						begin = false;
						break;
					}
				}
			} else
				inDiff = true;

			if (begin) {
				// skip leading spaces (tabs & newlines aren't allowed here)
				while (line[pos] == ' ')
					pos++;
			}
		}
	} else
		begin = true;

	// set styles for all qoute levels in the text to be inserted

	for (int32 pos = 0; pos < length;) {
		int32 next;
		if (begin && is_quote_char(line[pos])) {
			begin = false;

			while (pos < length && line[pos] != '\n') {
				// insert style for each quote level
				level++;

				bool search = true;
				for (next = pos + 1; next < length; next++) {
					if ((search && is_quote_char(line[next]))
						|| line[next] == '\n')
						break;
					else if (search && line[next] != ' ' && line[next] != '\t')
						search = false;
				}

				runs[index].offset = pos;
				runs[index].font = font;
				runs[index].color = level > 0
					? kQuoteColors[level % kNumQuoteColors] : kNormalTextColor;

				pos = next;
				if (++index >= maxStyles)
					break;
			}
		} else {
			if (begin) {
				if (!inDiff) {
					inDiff = !strncmp(&line[pos], "--- ", 4);
					wasDiff = false;
				}
				if (inDiff) {
					diffMode = diff_mode(line[pos]);
					if (diffMode < 0) {
						inDiff = false;
						wasDiff = true;
					}
				}
			}

			runs[index].offset = pos;
			runs[index].font = font;
			if (wasDiff)
				runs[index].color = kDiffColors[diff_mode('@') - 1];
			else if (diffMode <= 0) {
				runs[index].color = level > 0
					? kQuoteColors[level % kNumQuoteColors] : kNormalTextColor;
			} else
				runs[index].color = kDiffColors[diffMode - 1];

			begin = false;

			for (next = pos; next < length; next++) {
				if (line[next] == '\n') {
					begin = true;
					wasDiff = false;
					break;
				}
			}

			pos = next;
			index++;
		}

		if (pos < length)
			begin = line[pos] == '\n';

		if (begin) {
			pos++;
			level = 0;
			wasDiff = false;

			// skip one leading space (tabs & newlines aren't allowed here)
			if (!inDiff && pos < length && line[pos] == ' ')
				pos++;
		}

		if (index >= maxStyles)
			break;
	}
	style->count = index;

	if (context) {
		// update context for next run
		context->level = level;
		context->diff_mode = diffMode;
		context->begin = begin;
		context->in_diff = inDiff;
		context->was_diff = wasDiff;
	}
}


//	#pragma mark -


TextRunArray::TextRunArray(size_t entries)
	:
	fNumEntries(entries)
{
	fArray = (text_run_array *)malloc(sizeof(int32) + sizeof(text_run) * entries);
	if (fArray != NULL)
		fArray->count = 0;
}


TextRunArray::~TextRunArray()
{
	free(fArray);
}


//====================================================================
//	#pragma mark -


TContentView::TContentView(BRect rect, bool incoming, BFont* font,
	bool showHeader, bool coloredQuotes)
	:
	BView(rect, "m_content", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),

	fFocus(false),
	fIncoming(incoming)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fOffset = 12;

	BRect r(rect);
	r.OffsetTo(0, 0);
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r.bottom -= B_H_SCROLL_BAR_HEIGHT;
	r.top += 4;
	BRect text(r);
	text.OffsetTo(0, 0);
	text.InsetBy(5, 5);

	fTextView = new TTextView(r, text, fIncoming, this, font, showHeader,
		coloredQuotes);
	BScrollView *scroll = new BScrollView("", fTextView, B_FOLLOW_ALL, 0, true, true);
	AddChild(scroll);
}


void
TContentView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case CHANGE_FONT:
		{
			BFont *font;
			msg->FindPointer("font", (void **)&font);
			fTextView->UpdateFont(font);
			fTextView->Invalidate(Bounds());
			break;
		}

		case M_QUOTE:
		{
			int32 start, finish;
			fTextView->GetSelection(&start, &finish);
			fTextView->AddQuote(start, finish);
			break;
		}
		case M_REMOVE_QUOTE:
		{
			int32 start, finish;
			fTextView->GetSelection(&start, &finish);
			fTextView->RemoveQuote(start, finish);
			break;
		}

		case M_SIGNATURE:
		{
			entry_ref ref;
			msg->FindRef("ref", &ref);

			BFile file(&ref, B_READ_ONLY);
			if (file.InitCheck() == B_OK) {
				int32 start, finish;
				fTextView->GetSelection(&start, &finish);

				off_t size;
				file.GetSize(&size);
				if (size > 32768)	// safety against corrupt signatures
					break;

				char *signature = (char *)malloc(size);
				if (signature == NULL)
					break;
				ssize_t bytesRead = file.Read(signature, size);
				if (bytesRead < B_OK) {
					free (signature);
					break;
				}

				const char *text = fTextView->Text();
				int32 length = fTextView->TextLength();

				if (length && text[length - 1] != '\n') {
					fTextView->Select(length, length);

					char newLine = '\n';
					fTextView->Insert(&newLine, 1);

					length++;
				}

				fTextView->Select(length, length);
				fTextView->Insert(signature, bytesRead);
				fTextView->Select(length, length + bytesRead);
				fTextView->ScrollToSelection();

				fTextView->Select(start, finish);
				fTextView->ScrollToSelection();
				free (signature);
			} else {
				beep();
				(new BAlert("",
					B_TRANSLATE("An error occurred trying to open this "
						"signature."),
					B_TRANSLATE("Sorry")))->Go();
			}
			break;
		}

		case M_FIND:
			FindString(msg->FindString("findthis"));
			break;

		default:
			BView::MessageReceived(msg);
	}
}


void
TContentView::FindString(const char *str)
{
	int32	finish;
	int32	pass = 0;
	int32	start = 0;

	if (str == NULL)
		return;

	//
	//	Start from current selection or from the beginning of the pool
	//
	const char *text = fTextView->Text();
	int32 count = fTextView->TextLength();
	fTextView->GetSelection(&start, &finish);
	if (start != finish)
		start = finish;
	if (!count || text == NULL)
		return;

	//
	//	Do the find
	//
	while (pass < 2) {
		long found = -1;
		char lc = tolower(str[0]);
		char uc = toupper(str[0]);
		for (long i = start; i < count; i++) {
			if (text[i] == lc || text[i] == uc) {
				const char *s = str;
				const char *t = text + i;
				while (*s && (tolower(*s) == tolower(*t))) {
					s++;
					t++;
				}
				if (*s == 0) {
					found = i;
					break;
				}
			}
		}

		//
		//	Select the text if it worked
		//
		if (found != -1) {
			Window()->Activate();
			fTextView->Select(found, found + strlen(str));
			fTextView->ScrollToSelection();
			fTextView->MakeFocus(true);
			return;
		}
		else if (start) {
			start = 0;
			text = fTextView->Text();
			count = fTextView->TextLength();
			pass++;
		} else {
			beep();
			return;
		}
	}
}


void
TContentView::Focus(bool focus)
{
	if (fFocus != focus) {
		fFocus = focus;
		Draw(Frame());
	}
}


void
TContentView::FrameResized(float /* width */, float /* height */)
{
	BRect r(fTextView->Bounds());
	r.OffsetTo(0, 0);
	r.InsetBy(5, 5);
	fTextView->SetTextRect(r);
}


//====================================================================
//	#pragma mark -


TTextView::TTextView(BRect frame, BRect text, bool incoming, TContentView *view,
	BFont *font, bool showHeader, bool coloredQuotes)
	:
	BTextView(frame, "", text, B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE),

	fHeader(showHeader),
	fColoredQuotes(coloredQuotes),
	fReady(false),
	fYankBuffer(NULL),
	fLastPosition(-1),
	fMail(NULL),
	fFont(font),
	fParent(view),
	fStopLoading(false),
	fThread(0),
	fPanel(NULL),
	fIncoming(incoming),
	fSpellCheck(false),
	fRaw(false),
	fCursor(false),
	fFirstSpellMark(NULL)
{
	fStopSem = create_sem(1, "reader_sem");
	SetStylable(true);

	fEnclosures = new BList();

	// Enclosure pop up menu
	fEnclosureMenu = new BPopUpMenu("Enclosure", false, false);
	fEnclosureMenu->SetFont(be_plain_font);
	fEnclosureMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Save attachment" B_UTF8_ELLIPSIS),	new BMessage(M_SAVE)));
	fEnclosureMenu->AddItem(new BMenuItem(B_TRANSLATE("Open attachment"),
		new BMessage(M_OPEN)));

	// Hyperlink pop up menu
	fLinkMenu = new BPopUpMenu("Link", false, false);
	fLinkMenu->SetFont(be_plain_font);
	fLinkMenu->AddItem(new BMenuItem(B_TRANSLATE("Open this link"),
		new BMessage(M_OPEN)));
	fLinkMenu->AddItem(new BMenuItem(B_TRANSLATE("Copy link location"),
		new BMessage(M_COPY)));

	SetDoesUndo(true);

	//Undo function
	fUndoBuffer.On();
	fInputMethodUndoBuffer.On();
	fUndoState.replaced = false;
	fUndoState.deleted = false;
	fInputMethodUndoState.active = false;
	fInputMethodUndoState.replace = false;
}


TTextView::~TTextView()
{
	ClearList();
	delete fPanel;

	if (fYankBuffer)
		free(fYankBuffer);

	delete_sem(fStopSem);
}


void
TTextView::UpdateFont(const BFont* newFont)
{
	fFont = *newFont;

	// update the text run array safely with new font
	text_run_array *runArray = RunArray(0, LONG_MAX);
	for (int i = 0; i < runArray->count; i++)
		runArray->runs[i].font = *newFont;

	SetRunArray(0, LONG_MAX, runArray);
	FreeRunArray(runArray);
}


void
TTextView::AttachedToWindow()
{
	BTextView::AttachedToWindow();
	fFont.SetSpacing(B_FIXED_SPACING);
	SetFontAndColor(&fFont);

	if (fMail != NULL) {
		LoadMessage(fMail, false, NULL);
		if (fIncoming)
			MakeEditable(false);
	}
}


void
TTextView::KeyDown(const char *key, int32 count)
{
	char		raw;
	int32		end;
	int32 		start;
	uint32		mods;
	BMessage	*msg;
	int32		textLen = TextLength();

	msg = Window()->CurrentMessage();
	mods = msg->FindInt32("modifiers");

	switch (key[0])
	{
		case B_HOME:
			if (IsSelectable())
			{
				if (IsEditable())
					BTextView::KeyDown(key, count);
				else
				{
					// scroll to the beginning
					Select(0, 0);
					ScrollToSelection();
				}
			}
			break;

		case B_END:
			if (IsSelectable())
			{
				if (IsEditable())
					BTextView::KeyDown(key, count);
				else
				{
					// scroll to the end
					int32 length = TextLength();
					Select(length, length);
					ScrollToSelection();
				}
			}
			break;

		case 0x02:						// ^b - back 1 char
			if (IsSelectable())
			{
				GetSelection(&start, &end);
				while (!IsInitialUTF8Byte(ByteAt(--start)))
				{
					if (start < 0)
					{
						start = 0;
						break;
					}
				}
				if (start >= 0)
				{
					Select(start, start);
					ScrollToSelection();
				}
			}
			break;

		case B_DELETE:
			if (IsSelectable())
			{
				if ((key[0] == B_DELETE) || (mods & B_CONTROL_KEY))	// ^d
				{
					if (IsEditable())
					{
						GetSelection(&start, &end);
						if (start != end)
							Delete();
						else
						{
							for (end = start + 1; !IsInitialUTF8Byte(ByteAt(end)); end++)
							{
								if (end > textLen)
								{
									end = textLen;
									break;
								}
							}
							Select(start, end);
							Delete();
						}
					}
				}
				else
					Select(textLen, textLen);
				ScrollToSelection();
			}
			break;

		case 0x05:						// ^e - end of line
			if ((IsSelectable()) && (mods & B_CONTROL_KEY))
			{
				if (CurrentLine() == CountLines() - 1)
					Select(TextLength(), TextLength());
				else
				{
					GoToLine(CurrentLine() + 1);
					GetSelection(&start, &end);
					Select(start - 1, start - 1);
				}
			}
			break;

		case 0x06:						// ^f - forward 1 char
			if (IsSelectable())
			{
				GetSelection(&start, &end);
				if (end > start)
					start = end;
				else
				{
					for (end = start + 1; !IsInitialUTF8Byte(ByteAt(end)); end++)
					{
						if (end > textLen)
						{
							end = textLen;
							break;
						}
					}
					start = end;
				}
				Select(start, start);
				ScrollToSelection();
			}
			break;

		case 0x0e:						// ^n - next line
			if (IsSelectable())
			{
				raw = B_DOWN_ARROW;
				BTextView::KeyDown(&raw, 1);
			}
			break;

		case 0x0f:						// ^o - open line
			if (IsEditable())
			{
				GetSelection(&start, &end);
				Delete();

				char newLine = '\n';
				Insert(&newLine, 1);
				Select(start, start);
				ScrollToSelection();
			}
			break;

		case B_PAGE_UP:
			if (mods & B_CONTROL_KEY) {	// ^k kill text from cursor to e-o-line
				if (IsEditable()) {
					GetSelection(&start, &end);
					if ((start != fLastPosition) && (fYankBuffer)) {
						free(fYankBuffer);
						fYankBuffer = NULL;
					}
					fLastPosition = start;
					if (CurrentLine() < (CountLines() - 1)) {
						GoToLine(CurrentLine() + 1);
						GetSelection(&end, &end);
						end--;
					}
					else
						end = TextLength();
					if (end < start)
						break;
					if (start == end)
						end++;
					Select(start, end);
					if (fYankBuffer) {
						fYankBuffer = (char *)realloc(fYankBuffer,
									 strlen(fYankBuffer) + (end - start) + 1);
						GetText(start, end - start,
							    &fYankBuffer[strlen(fYankBuffer)]);
					} else {
						fYankBuffer = (char *)malloc(end - start + 1);
						GetText(start, end - start, fYankBuffer);
					}
					Delete();
					ScrollToSelection();
				}
				break;
			}

			BTextView::KeyDown(key, count);
			break;

		case 0x10:						// ^p goto previous line
			if (IsSelectable()) {
				raw = B_UP_ARROW;
				BTextView::KeyDown(&raw, 1);
			}
			break;

		case 0x19:						// ^y yank text
			if ((IsEditable()) && (fYankBuffer)) {
				Delete();
				Insert(fYankBuffer);
				ScrollToSelection();
			}
			break;

		default:
			BTextView::KeyDown(key, count);
	}
}


void
TTextView::MakeFocus(bool focus)
{
	if (!focus) {
		// ToDo: can someone please translate this? Otherwise I will remove it - axeld.
		// MakeFocus(false) は、IM も Inactive になり、そのまま確定される。
		// しかしこの場合、input_server が B_INPUT_METHOD_EVENT(B_INPUT_METHOD_STOPPED)
		// を送ってこないまま矛盾してしまうので、やむを得ずここでつじつまあわせ処理している。
		fInputMethodUndoState.active = false;
		// fInputMethodUndoBufferに溜まっている最後のデータがK_INSERTEDなら（確定）正規のバッファへ追加
		if (fInputMethodUndoBuffer.CountItems() > 0) {
			KUndoItem *item = fInputMethodUndoBuffer.ItemAt(fInputMethodUndoBuffer.CountItems() - 1);
			if (item->History == K_INSERTED) {
				fUndoBuffer.MakeNewUndoItem();
				fUndoBuffer.AddUndo(item->RedoText, item->Length, item->Offset, item->History, item->CursorPos);
				fUndoBuffer.MakeNewUndoItem();
			}
			fInputMethodUndoBuffer.MakeEmpty();
		}
	}
	BTextView::MakeFocus(focus);

	fParent->Focus(focus);
}


void
TTextView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_SIMPLE_DATA:
		{
			if (fIncoming)
				break;

			BMessage message(REFS_RECEIVED);
			bool isEnclosure = false;
			bool inserted = false;

			off_t len = 0;
			int32 end;
			int32 start;

			int32 index = 0;
			entry_ref ref;
			while (msg->FindRef("refs", index++, &ref) == B_OK) {
				BFile file(&ref, B_READ_ONLY);
				if (file.InitCheck() == B_OK) {
					BNodeInfo node(&file);
					char type[B_FILE_NAME_LENGTH];
					node.GetType(type);

					off_t size = 0;
					file.GetSize(&size);

					if (!strncasecmp(type, "text/", 5) && size > 0) {
						len += size;
						char *text = (char *)malloc(size);
						if (text == NULL) {
							puts("no memory!");
							return;
						}
						if (file.Read(text, size) < B_OK) {
							puts("could not read from file");
							free(text);
							continue;
						}
						if (!inserted) {
							GetSelection(&start, &end);
							Delete();
							inserted = true;
						}

						int32 offset = 0;
						for (int32 loop = 0; loop < size; loop++) {
							if (text[loop] == '\n') {
								Insert(&text[offset], loop - offset + 1);
								offset = loop + 1;
							} else if (text[loop] == '\r') {
								text[loop] = '\n';
								Insert(&text[offset], loop - offset + 1);
								if ((loop + 1 < size)
										&& (text[loop + 1] == '\n'))
									loop++;
								offset = loop + 1;
							}
						}
						free(text);
					} else {
						isEnclosure = true;
						message.AddRef("refs", &ref);
					}
				}
			}

			if (index == 1) {
				// message doesn't contain any refs - maybe the parent class likes it
				BTextView::MessageReceived(msg);
				break;
			}

			if (inserted)
				Select(start, start + len);
			if (isEnclosure)
				Window()->PostMessage(&message, Window());
			break;
		}

		case M_HEADER:
			msg->FindBool("header", &fHeader);
			SetText(NULL);
			LoadMessage(fMail, false, NULL);
			break;

		case M_RAW:
			StopLoad();

			msg->FindBool("raw", &fRaw);
			SetText(NULL);
			LoadMessage(fMail, false, NULL);
			break;

		case M_SELECT:
			if (IsSelectable())
				Select(0, TextLength());
			break;

		case M_SAVE:
			Save(msg);
			break;

		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (msg->FindInt32("opcode", &opcode) == B_NO_ERROR) {
				dev_t device;
				if (msg->FindInt32("device", &device) < B_OK)
					break;
				ino_t inode;
				if (msg->FindInt64("node", &inode) < B_OK)
					break;

				hyper_text *enclosure;
				for (int32 index = 0;
						(enclosure = (hyper_text *)fEnclosures->ItemAt(index++)) != NULL;) {
					if (device == enclosure->node.device
						&& inode == enclosure->node.node) {
						if (opcode == B_ENTRY_REMOVED) {
							enclosure->saved = false;
							enclosure->have_ref = false;
						} else if (opcode == B_ENTRY_MOVED) {
							enclosure->ref.device = device;
							msg->FindInt64("to directory", &enclosure->ref.directory);

							const char *name;
							msg->FindString("name", &name);
							enclosure->ref.set_name(name);
						}
						break;
					}
				}
			}
			break;
		}

		//
		// Tracker has responded to a BMessage that was dragged out of
		// this email message.  It has created a file for us, we just have to
		// put the stuff in it.
		//
		case B_COPY_TARGET:
		{
			BMessage data;
			if (msg->FindMessage("be:originator-data", &data) == B_OK) {
				entry_ref directory;
				const char *name;
				hyper_text *enclosure;

				if (data.FindPointer("enclosure", (void **)&enclosure) == B_OK
					&& msg->FindString("name", &name) == B_OK
					&& msg->FindRef("directory", &directory) == B_OK) {
					switch (enclosure->type) {
						case TYPE_ENCLOSURE:
						case TYPE_BE_ENCLOSURE:
						{
							//
							//	Enclosure.  Decode the data and write it out.
							//
							BMessage saveMsg(M_SAVE);
							saveMsg.AddString("name", name);
							saveMsg.AddRef("directory", &directory);
							saveMsg.AddPointer("enclosure", enclosure);
							Save(&saveMsg, false);
							break;
						}

						case TYPE_URL:
						{
							const char *replyType;
							if (msg->FindString("be:filetypes", &replyType) != B_OK)
								// drag recipient didn't ask for any specific type,
								// create a bookmark file as default
								replyType = "application/x-vnd.Be-bookmark";

							BDirectory dir(&directory);
							BFile file(&dir, name, B_READ_WRITE);
							if (file.InitCheck() == B_OK) {
								if (strcmp(replyType, "application/x-vnd.Be-bookmark") == 0) {
									// we got a request to create a bookmark, stuff
									// it with the url attribute
									file.WriteAttr("META:url", B_STRING_TYPE, 0,
													enclosure->name, strlen(enclosure->name) + 1);
								} else if (strcasecmp(replyType, "text/plain") == 0) {
									// create a plain text file, stuff it with
									// the url as text
									file.Write(enclosure->name, strlen(enclosure->name));
								}

								BNodeInfo fileInfo(&file);
								fileInfo.SetType(replyType);
							}
							break;
						}

						case TYPE_MAILTO:
						{
							//
							//	Add some attributes to the already created
							//	person file.  Strip out the 'mailto:' if
							//  possible.
							//
							char *addrStart = enclosure->name;
							while (true) {
								if (*addrStart == ':') {
									addrStart++;
									break;
								}

								if (*addrStart == '\0') {
									addrStart = enclosure->name;
									break;
								}

								addrStart++;
							}

							const char *replyType;
							if (msg->FindString("be:filetypes", &replyType) != B_OK)
								// drag recipient didn't ask for any specific type,
								// create a bookmark file as default
								replyType = "application/x-vnd.Be-bookmark";

							BDirectory dir(&directory);
							BFile file(&dir, name, B_READ_WRITE);
							if (file.InitCheck() == B_OK) {
								if (!strcmp(replyType, "application/x-person")) {
									// we got a request to create a bookmark, stuff
									// it with the address attribute
									file.WriteAttr("META:email", B_STRING_TYPE, 0,
									  addrStart, strlen(enclosure->name) + 1);
								} else if (!strcasecmp(replyType, "text/plain")) {
									// create a plain text file, stuff it with the
									// email as text
									file.Write(addrStart, strlen(addrStart));
								}

								BNodeInfo fileInfo(&file);
								fileInfo.SetType(replyType);
							}
							break;
						}
					}
				} else {
					//
					// Assume this is handled by BTextView...
					// (Probably drag clipping.)
					//
					BTextView::MessageReceived(msg);
				}
			}
			break;
		}

		case B_INPUT_METHOD_EVENT:
		{
			int32 im_op;
			if (msg->FindInt32("be:opcode", &im_op) == B_OK){
				switch (im_op) {
					case B_INPUT_METHOD_STARTED:
						fInputMethodUndoState.replace = true;
						fInputMethodUndoState.active = true;
						break;
					case B_INPUT_METHOD_STOPPED:
						fInputMethodUndoState.active = false;
						if (fInputMethodUndoBuffer.CountItems() > 0) {
							KUndoItem *undo = fInputMethodUndoBuffer.ItemAt(
								fInputMethodUndoBuffer.CountItems() - 1);
							if (undo->History == K_INSERTED){
								fUndoBuffer.MakeNewUndoItem();
								fUndoBuffer.AddUndo(undo->RedoText, undo->Length,
									undo->Offset, undo->History, undo->CursorPos);
								fUndoBuffer.MakeNewUndoItem();
							}
							fInputMethodUndoBuffer.MakeEmpty();
						}
						break;
					case B_INPUT_METHOD_CHANGED:
						fInputMethodUndoState.active = true;
						break;
					case B_INPUT_METHOD_LOCATION_REQUEST:
						fInputMethodUndoState.active = true;
						break;
				}
			}
			BTextView::MessageReceived(msg);
			break;
		}

		case M_REDO:
			Redo();
			break;

		default:
			BTextView::MessageReceived(msg);
	}
}


void
TTextView::MouseDown(BPoint where)
{
	if (IsEditable()) {
		BPoint point;
		uint32 buttons;
		GetMouse(&point, &buttons);
		if (gDictCount && (buttons == B_SECONDARY_MOUSE_BUTTON)) {
			int32 offset, start, end, length;
			const char *text = Text();
			offset = OffsetAt(where);
			if (isalpha(text[offset])) {
				length = TextLength();

				//Find start and end of word
				//FindSpellBoundry(length, offset, &start, &end);

				char c;
				bool isAlpha, isApost, isCap;
				int32 first;

				for (first = offset;
					(first >= 0) && (((c = text[first]) == '\'') || isalpha(c));
					first--) {}
				isCap = isupper(text[++first]);

				for (start = offset, c = text[start], isAlpha = isalpha(c), isApost = (c=='\'');
					(start >= 0) && (isAlpha || (isApost
					&& (((c = text[start+1]) != 's') || !isCap) && isalpha(c)
					&& isalpha(text[start-1])));
					start--, c = text[start], isAlpha = isalpha(c), isApost = (c == '\'')) {}
				start++;

				for (end = offset, c = text[end], isAlpha = isalpha(c), isApost = (c == '\'');
					(end < length) && (isAlpha || (isApost
					&& (((c = text[end + 1]) != 's') || !isCap) && isalpha(c)));
					end++, c = text[end], isAlpha = isalpha(c), isApost = (c == '\'')) {}

				length = end - start;
				BString srcWord;
				srcWord.SetTo(text + start, length);

				bool		foundWord = false;
				BList 		matches;
				BString 	*string;

				BMenuItem *menuItem;
				BPopUpMenu menu("Words", false, false);

				int32 matchCount;
				for (int32 i = 0; i < gDictCount; i++)
					matchCount = gWords[i]->FindBestMatches(&matches,
						srcWord.String());

				if (matches.CountItems()) {
					sort_word_list(&matches, srcWord.String());
					for (int32 i = 0; (string = (BString *)matches.ItemAt(i)) != NULL; i++) {
						menu.AddItem((menuItem = new BMenuItem(string->String(), NULL)));
						if (!strcasecmp(string->String(), srcWord.String())) {
							menuItem->SetEnabled(false);
							foundWord = true;
						}
						delete string;
					}
				} else {
					menuItem = new BMenuItem(B_TRANSLATE("No matches"), NULL);
					menuItem->SetEnabled(false);
					menu.AddItem(menuItem);
				}

				BMenuItem *addItem = NULL;
				if (!foundWord && gUserDict >= 0) {
					menu.AddSeparatorItem();
					addItem = new BMenuItem(B_TRANSLATE("Add"), NULL);
					menu.AddItem(addItem);
				}

				point = ConvertToScreen(where);
				if ((menuItem = menu.Go(point, false, false)) != NULL) {
					if (menuItem == addItem) {
						BString newItem(srcWord.String());
						newItem << "\n";
						gWords[gUserDict]->InitIndex();
						gExactWords[gUserDict]->InitIndex();
						gUserDictFile->Write(newItem.String(), newItem.Length());
						gWords[gUserDict]->BuildIndex();
						gExactWords[gUserDict]->BuildIndex();

						if (fSpellCheck)
							CheckSpelling(0, TextLength());
					} else {
						int32 len = strlen(menuItem->Label());
						Select(start, start);
						Delete(start, end);
						Insert(start, menuItem->Label(), len);
						Select(start+len, start+len);
					}
				}
			}
			return;
		} else if (fSpellCheck && IsEditable()) {
			int32 start, end;

			GetSelection(&start, &end);
			FindSpellBoundry(1, start, &start, &end);
			CheckSpelling(start, end);
		}
	} else {
		// is not editable, look for enclosures/links

		int32 clickOffset = OffsetAt(where);
		int32 items = fEnclosures->CountItems();
		for (int32 loop = 0; loop < items; loop++) {
			hyper_text *enclosure = (hyper_text*) fEnclosures->ItemAt(loop);
			if (clickOffset < enclosure->text_start || clickOffset >= enclosure->text_end)
				continue;

			//
			// The user is clicking on this attachment
			//

			int32 start;
			int32 finish;
			Select(enclosure->text_start, enclosure->text_end);
			GetSelection(&start, &finish);
			Window()->UpdateIfNeeded();

			bool drag = false;
			bool held = false;
			uint32 buttons = 0;
			if (Window()->CurrentMessage()) {
				Window()->CurrentMessage()->FindInt32("buttons",
				  (int32 *) &buttons);
			}

			//
			// If this is the primary button, wait to see if the user is going
			// to single click, hold, or drag.
			//
			if (buttons != B_SECONDARY_MOUSE_BUTTON) {
				BPoint point = where;
				bigtime_t popupDelay;
				get_click_speed(&popupDelay);
				popupDelay *= 2;
				popupDelay += system_time();
				while (buttons && abs((int)(point.x - where.x)) < 4
					&& abs((int)(point.y - where.y)) < 4
					&& system_time() < popupDelay) {
					snooze(10000);
					GetMouse(&point, &buttons);
				}

				if (system_time() < popupDelay) {
					//
					// The user either dragged this or released the button.
					// check if it was dragged.
					//
					if (!(abs((int)(point.x - where.x)) < 4
						&& abs((int)(point.y - where.y)) < 4) && buttons)
						drag = true;
				} else  {
					//
					//	The user held the button down.
					//
					held = true;
				}
			}

			//
			//	If the user has right clicked on this menu,
			// 	or held the button down on it for a while,
			//	pop up a context menu.
			//
			if (buttons == B_SECONDARY_MOUSE_BUTTON || held) {
				//
				// Right mouse click... Display a menu
				//
				BPoint point = where;
				ConvertToScreen(&point);

				BMenuItem *item;
				if ((enclosure->type != TYPE_ENCLOSURE)
					&& (enclosure->type != TYPE_BE_ENCLOSURE))
					item = fLinkMenu->Go(point, true);
				else
					item = fEnclosureMenu->Go(point, true);

				BMessage *msg;
				if (item && (msg = item->Message()) != NULL) {
					if (msg->what == M_SAVE) {
						if (fPanel)
							fPanel->SetEnclosure(enclosure);
						else {
							fPanel = new TSavePanel(enclosure, this);
							fPanel->Window()->Show();
						}
					} else if (msg->what == M_COPY) {
						// copy link location to clipboard

						if (be_clipboard->Lock()) {
							be_clipboard->Clear();

							BMessage *clip;
							if ((clip = be_clipboard->Data()) != NULL) {
								clip->AddData("text/plain", B_MIME_TYPE,
									enclosure->name, strlen(enclosure->name));
								be_clipboard->Commit();
							}
							be_clipboard->Unlock();
						}
					} else
						Open(enclosure);
				}
			} else {
				//
				// Left button.  If the user single clicks, open this link.
				// Otherwise, initiate a drag.
				//
				if (drag) {
					BMessage dragMessage(B_SIMPLE_DATA);
					dragMessage.AddInt32("be:actions", B_COPY_TARGET);
					dragMessage.AddString("be:types", B_FILE_MIME_TYPE);
					switch (enclosure->type) {
						case TYPE_BE_ENCLOSURE:
						case TYPE_ENCLOSURE:
							//
							// Attachment.  The type is specified in the message.
							//
							dragMessage.AddString("be:types", B_FILE_MIME_TYPE);
							dragMessage.AddString("be:filetypes",
								enclosure->content_type ? enclosure->content_type : "");
							dragMessage.AddString("be:clip_name", enclosure->name);
							break;

						case TYPE_URL:
							//
							// URL.  The user can drag it into the tracker to
							// create a bookmark file.
							//
							dragMessage.AddString("be:types", B_FILE_MIME_TYPE);
							dragMessage.AddString("be:filetypes",
							  "application/x-vnd.Be-bookmark");
							dragMessage.AddString("be:filetypes", "text/plain");
							dragMessage.AddString("be:clip_name", "Bookmark");

							dragMessage.AddString("be:url", enclosure->name);
							break;

						case TYPE_MAILTO:
							//
							// Mailto address.  The user can drag it into the
							// tracker to create a people file.
							//
							dragMessage.AddString("be:types", B_FILE_MIME_TYPE);
							dragMessage.AddString("be:filetypes",
							  "application/x-person");
							dragMessage.AddString("be:filetypes", "text/plain");
							dragMessage.AddString("be:clip_name", "Person");

							dragMessage.AddString("be:email", enclosure->name);
							break;

						default:
							//
							// Otherwise it doesn't have a type that I know how
							// to save.  It won't have any types and if any
							// program wants to accept it, more power to them.
							// (tracker won't.)
							//
							dragMessage.AddString("be:clip_name", "Hyperlink");
					}

					BMessage data;
					data.AddPointer("enclosure", enclosure);
					dragMessage.AddMessage("be:originator-data", &data);

					BRegion selectRegion;
					GetTextRegion(start, finish, &selectRegion);
					DragMessage(&dragMessage, selectRegion.Frame(), this);
				} else {
					//
					//	User Single clicked on the attachment.  Open it.
					//
					Open(enclosure);
				}
			}
			return;
		}
	}
	BTextView::MouseDown(where);
}


void
TTextView::MouseMoved(BPoint where, uint32 code, const BMessage *msg)
{
	int32 start = OffsetAt(where);

	for (int32 loop = fEnclosures->CountItems(); loop-- > 0;) {
		hyper_text *enclosure = (hyper_text *)fEnclosures->ItemAt(loop);
		if ((start >= enclosure->text_start) && (start < enclosure->text_end)) {
			if (!fCursor)
				SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
			fCursor = true;
			return;
		}
	}

	if (fCursor) {
		SetViewCursor(B_CURSOR_I_BEAM);
		fCursor = false;
	}

	BTextView::MouseMoved(where, code, msg);
}


void
TTextView::ClearList()
{
	hyper_text *enclosure;
	while ((enclosure = (hyper_text *)fEnclosures->FirstItem()) != NULL) {
		fEnclosures->RemoveItem(enclosure);

		if (enclosure->name)
			free(enclosure->name);
		if (enclosure->content_type)
			free(enclosure->content_type);
		if (enclosure->encoding)
			free(enclosure->encoding);
		if (enclosure->have_ref && !enclosure->saved) {
			BEntry entry(&enclosure->ref);
			entry.Remove();
		}

		watch_node(&enclosure->node, B_STOP_WATCHING, this);
		free(enclosure);
	}
}


void
TTextView::LoadMessage(BEmailMessage *mail, bool quoteIt, const char *text)
{
	StopLoad();

	fMail = mail;

	ClearList();

	MakeSelectable(true);
	MakeEditable(false);
	if (text)
		Insert(text, strlen(text));

	//attr_info attrInfo;
	TTextView::Reader *reader = new TTextView::Reader(fHeader, fRaw, quoteIt, fIncoming,
				text != NULL, true,
				// I removed the following, because I absolutely can't imagine why it's
				// there (the mail kit should be able to deal with non-compliant mails)
				// -- axeld.
				// fFile->GetAttrInfo(B_MAIL_ATTR_MIME, &attrInfo) == B_OK,
				this, mail, fEnclosures, fStopSem);

	resume_thread(fThread = spawn_thread(Reader::Run, "reader", B_NORMAL_PRIORITY, reader));
}


void
TTextView::Open(hyper_text *enclosure)
{
	switch (enclosure->type) {
		case TYPE_URL:
		{
			const struct {const char *urlType, *handler; } handlerTable[] = {
				{"http",	B_URL_HTTP},
				{"https",	B_URL_HTTPS},
				{"ftp",		B_URL_FTP},
				{"gopher",	B_URL_GOPHER},
				{"mailto",	B_URL_MAILTO},
				{"news",	B_URL_NEWS},
				{"nntp",	B_URL_NNTP},
				{"telnet",	B_URL_TELNET},
				{"rlogin",	B_URL_RLOGIN},
				{"tn3270",	B_URL_TN3270},
				{"wais",	B_URL_WAIS},
				{"file",	B_URL_FILE},
				{NULL,		NULL}
			};
			const char *handlerToLaunch = NULL;

			const char *colonPos = strchr(enclosure->name, ':');
			if (colonPos) {
				int urlTypeLength = colonPos - enclosure->name;

				for (int32 index = 0; handlerTable[index].urlType; index++) {
					if (!strncasecmp(enclosure->name,
							handlerTable[index].urlType, urlTypeLength)) {
						handlerToLaunch = handlerTable[index].handler;
						break;
					}
				}
			}
			if (handlerToLaunch) {
				entry_ref appRef;
				if (be_roster->FindApp(handlerToLaunch, &appRef) != B_OK)
					handlerToLaunch = NULL;
			}
			if (!handlerToLaunch)
				handlerToLaunch = "application/x-vnd.Be-Bookmark";

			status_t result = be_roster->Launch(handlerToLaunch, 1, &enclosure->name);
			if (result != B_NO_ERROR && result != B_ALREADY_RUNNING) {
				beep();
				(new BAlert("",
					B_TRANSLATE("There is no installed handler for "
						"URL links."),
					"Sorry"))->Go();
			}
			break;
		}

		case TYPE_MAILTO:
			if (be_roster->Launch(B_MAIL_TYPE, 1, &enclosure->name) < B_OK) {
				char *argv[] = {(char *)"Mail", enclosure->name};
				be_app->ArgvReceived(2, argv);
			}
			break;

		case TYPE_ENCLOSURE:
		case TYPE_BE_ENCLOSURE:
			if (!enclosure->have_ref) {
				BPath path;
				if (find_directory(B_COMMON_TEMP_DIRECTORY, &path) == B_NO_ERROR) {
					BDirectory dir(path.Path());
					if (dir.InitCheck() == B_NO_ERROR) {
						char name[B_FILE_NAME_LENGTH];
						char baseName[B_FILE_NAME_LENGTH];
						strcpy(baseName, enclosure->name ? enclosure->name : "enclosure");
						strcpy(name, baseName);
						for (int32 index = 0; dir.Contains(name); index++)
							sprintf(name, "%s_%ld", baseName, index);

						BEntry entry(path.Path());
						entry_ref ref;
						entry.GetRef(&ref);

						BMessage save(M_SAVE);
						save.AddRef("directory", &ref);
						save.AddString("name", name);
						save.AddPointer("enclosure", enclosure);
						if (Save(&save) != B_NO_ERROR)
							break;
						enclosure->saved = false;
					}
				}
			}

			BMessenger tracker("application/x-vnd.Be-TRAK");
			if (tracker.IsValid()) {
				BMessage openMsg(B_REFS_RECEIVED);
				openMsg.AddRef("refs", &enclosure->ref);
				tracker.SendMessage(&openMsg);
			}
			break;
	}
}


status_t
TTextView::Save(BMessage *msg, bool makeNewFile)
{
	const char		*name;
	entry_ref		ref;
	BFile			file;
	BPath			path;
	hyper_text		*enclosure;
	status_t		result = B_NO_ERROR;
	char 			entry_name[B_FILE_NAME_LENGTH];

	msg->FindString("name", &name);
	msg->FindRef("directory", &ref);
	msg->FindPointer("enclosure", (void **)&enclosure);

	BDirectory dir;
	dir.SetTo(&ref);
	result = dir.InitCheck();

	if (result == B_OK) {
		if (makeNewFile) {
			//
			// Search for the file and delete it if it already exists.
			// (It may not, that's ok.)
			//
			BEntry entry;
			if (dir.FindEntry(name, &entry) == B_NO_ERROR)
				entry.Remove();

			if ((enclosure->have_ref) && (!enclosure->saved)) {
				entry.SetTo(&enclosure->ref);

				//
				// Added true arg and entry_name so MoveTo clobbers as
				// before. This may not be the correct behaviour, but
				// it's the preserved behaviour.
				//
				entry.GetName(entry_name);
				result = entry.MoveTo(&dir, entry_name, true);
				if (result == B_NO_ERROR) {
					entry.Rename(name);
					entry.GetRef(&enclosure->ref);
					entry.GetNodeRef(&enclosure->node);
					enclosure->saved = true;
					return result;
				}
			}

			if (result == B_NO_ERROR) {
				result = dir.CreateFile(name, &file);
				if (result == B_NO_ERROR && enclosure->content_type) {
					char type[B_MIME_TYPE_LENGTH];

					if (!strcasecmp(enclosure->content_type, "message/rfc822"))
						strcpy(type, "text/x-email");
					else if (!strcasecmp(enclosure->content_type, "message/delivery-status"))
						strcpy(type, "text/plain");
					else
						strcpy(type, enclosure->content_type);

					BNodeInfo info(&file);
					info.SetType(type);
				}
			}
		} else {
			//
			// 	This file was dragged into the tracker or desktop.  The file
			//	already exists.
			//
			result = file.SetTo(&dir, name, B_WRITE_ONLY);
		}
	}

	if (enclosure->component == NULL)
		result = B_ERROR;

	if (result == B_NO_ERROR) {
		//
		// Write the data
		//
		enclosure->component->GetDecodedData(&file);

		BEntry entry;
		dir.FindEntry(name, &entry);
		entry.GetRef(&enclosure->ref);
		enclosure->have_ref = true;
		enclosure->saved = true;
		entry.GetPath(&path);
		update_mime_info(path.Path(), false, true,
			!cistrcmp("application/octet-stream", enclosure->content_type ? enclosure->content_type : B_EMPTY_STRING));
		entry.GetNodeRef(&enclosure->node);
		watch_node(&enclosure->node, B_WATCH_NAME, this);
	}

	if (result != B_NO_ERROR) {
		beep();
		(new BAlert("", B_TRANSLATE("An error occurred trying to save "
				"the attachment."),
			B_TRANSLATE("Sorry")))->Go();
	}

	return result;
}


void
TTextView::StopLoad()
{
	Window()->Unlock();

	thread_info	info;
	if (fThread != 0 && get_thread_info(fThread, &info) == B_NO_ERROR) {
		fStopLoading = true;
		acquire_sem(fStopSem);
		int32 result;
		wait_for_thread(fThread, &result);
		fThread = 0;
		release_sem(fStopSem);
		fStopLoading = false;
	}

	Window()->Lock();
}


void
TTextView::AddAsContent(BEmailMessage *mail, bool wrap, uint32 charset, mail_encoding encoding)
{
	if (mail == NULL)
		return;

	int32 textLength = TextLength();
	const char *text = Text();

	BTextMailComponent *body = mail->Body();
	if (body == NULL) {
		if (mail->SetBody(body = new BTextMailComponent()) < B_OK)
			return;
	}
	body->SetEncoding(encoding, charset);

	// Just add the text as a whole if we can, or ...
	if (!wrap) {
		body->AppendText(text);
		return;
	}

	// ... do word wrapping.

	BWindow	*window = Window();
	char *saveText = strdup(text);
	BRect saveTextRect = TextRect();

	// do this before we start messing with the fonts
	// the user will never know...
	window->DisableUpdates();
	Hide();
	BScrollBar *vScroller = ScrollBar(B_VERTICAL);
	BScrollBar *hScroller = ScrollBar(B_HORIZONTAL);
	if (vScroller != NULL)
		vScroller->SetTarget((BView *)NULL);
	if (hScroller != NULL)
		hScroller->SetTarget((BView *)NULL);

	// Temporarily set the font to a fixed width font for line wrapping
	// calculations.  If the font doesn't have as many of the symbols as
	// the preferred font, go back to using the user's preferred font.

	bool *boolArray;
	int missingCharactersFixedWidth = 0;
	int missingCharactersPreferredFont = 0;
	int32 numberOfCharacters;

	numberOfCharacters = BString(text).CountChars();
	if (numberOfCharacters > 0
		&& (boolArray = (bool *)malloc(sizeof(bool) * numberOfCharacters)) != NULL) {
		memset(boolArray, 0, sizeof (bool) * numberOfCharacters);
		be_fixed_font->GetHasGlyphs(text, numberOfCharacters, boolArray);
		for (int i = 0; i < numberOfCharacters; i++) {
			if (!boolArray[i])
				missingCharactersFixedWidth += 1;
		}

		memset(boolArray, 0, sizeof (bool) * numberOfCharacters);
		fFont.GetHasGlyphs(text, numberOfCharacters, boolArray);
		for (int i = 0; i < numberOfCharacters; i++) {
			if (!boolArray[i])
				missingCharactersPreferredFont += 1;
		}

		free(boolArray);
	}

	if (missingCharactersFixedWidth > missingCharactersPreferredFont)
		SetFontAndColor(0, textLength, &fFont);
	else // All things being equal, the fixed font is better for wrapping.
		SetFontAndColor(0, textLength, be_fixed_font);

	// calculate a text rect that is 72 columns wide
	BRect newTextRect = saveTextRect;
	newTextRect.right = newTextRect.left + be_fixed_font->StringWidth("m") * 72;
	SetTextRect(newTextRect);

	// hard-wrap, based on TextView's soft-wrapping
	int32 numLines = CountLines();
	bool spaceMoved = false;
	char *content = (char *)malloc(textLength + numLines * 72);	// more we'll ever need
	if (content != NULL) {
		int32 contentLength = 0;

		for (int32 i = 0; i < numLines; i++) {
			int32 startOffset = OffsetAt(i);
			if (spaceMoved) {
				startOffset++;
				spaceMoved = false;
			}
			int32 endOffset = OffsetAt(i + 1);
			int32 lineLength = endOffset - startOffset;

			// quick hack to not break URLs into several parts
			for (int32 pos = startOffset; pos < endOffset; pos++) {
				size_t urlLength;
				uint8 type = CheckForURL(text + pos, urlLength);
				if (type != 0)
					pos += urlLength;

				if (pos > endOffset) {
					// find first break character after the URL
					for (; text[pos]; pos++) {
						if (isalnum(text[pos]) || isspace(text[pos]))
							break;
					}
					if (text[pos] && isspace(text[pos]) && text[pos] != '\n')
						pos++;

					endOffset += pos - endOffset;
					lineLength = endOffset - startOffset;

					// insert a newline (and the same number of quotes) after the
					// URL to make sure the rest of the text is properly wrapped

					char buffer[64];
					if (text[pos] == '\n')
						buffer[0] = '\0';
					else
						strcpy(buffer, "\n");

					size_t quoteLength;
					CopyQuotes(text + startOffset, lineLength, buffer + strlen(buffer), quoteLength);

					Insert(pos, buffer, strlen(buffer));
					numLines = CountLines();
					text = Text();
					i++;
				}
			}
			if (text[endOffset - 1] != ' '
				&& text[endOffset - 1] != '\n'
				&& text[endOffset] == ' ') {
				// make sure spaces will be part of this line
				endOffset++;
				lineLength++;
				spaceMoved = true;
			}

			memcpy(content + contentLength, text + startOffset, lineLength);
			contentLength += lineLength;

			// add a newline to every line except for the ones
			// that already end in newlines, and the last line
			if ((text[endOffset - 1] != '\n') && (i < (numLines - 1))) {
				content[contentLength++] = '\n';

				// copy quote level of the first line
				size_t quoteLength;
				CopyQuotes(text + startOffset, lineLength, content + contentLength, quoteLength);
				contentLength += quoteLength;
			}
		}
		content[contentLength] = '\0';

		body->AppendText(content);
		free(content);
	}

	// reset the text rect and font
	SetTextRect(saveTextRect);
	SetText(saveText);
	free(saveText);
	SetFontAndColor(0, textLength, &fFont);

	// should be OK to hook these back up now
	if (vScroller != NULL)
		vScroller->SetTarget(this);
	if (hScroller != NULL)
		hScroller->SetTarget(this);

	Show();
	window->EnableUpdates();
}


//	#pragma mark -


TTextView::Reader::Reader(bool header, bool raw, bool quote, bool incoming,
		bool stripHeader, bool mime, TTextView *view, BEmailMessage *mail,
		BList *list, sem_id sem)
	:
	fHeader(header),
	fRaw(raw),
	fQuote(quote),
	fIncoming(incoming),
	fStripHeader(stripHeader),
	fMime(mime),
	fView(view),
	fMail(mail),
	fEnclosures(list),
	fStopSem(sem)
{
}


bool
TTextView::Reader::ParseMail(BMailContainer *container,
	BTextMailComponent *ignore)
{
	int32 count = 0;
	for (int32 i = 0; i < container->CountComponents(); i++) {
		if (fView->fStopLoading)
			return false;

		BMailComponent *component;
		if ((component = container->GetComponent(i)) == NULL) {
			if (fView->fStopLoading)
				return false;

			hyper_text *enclosure = (hyper_text *)malloc(sizeof(hyper_text));
			if (enclosure == NULL)
				return false;

			memset(enclosure, 0, sizeof(hyper_text));

			enclosure->type = TYPE_ENCLOSURE;

			const char *name = "\n<Attachment: could not handle>\n";

			fView->GetSelection(&enclosure->text_start, &enclosure->text_end);
			enclosure->text_start++;
			enclosure->text_end += strlen(name) - 1;

			Insert(name, strlen(name), true);
			fEnclosures->AddItem(enclosure);
			continue;
		}

		count++;
		if (component == ignore)
			continue;

		if (component->ComponentType() == B_MAIL_MULTIPART_CONTAINER) {
			BMIMEMultipartMailContainer *c = dynamic_cast<BMIMEMultipartMailContainer *>(container->GetComponent(i));
			ASSERT(c != NULL);

			if (!ParseMail(c, ignore))
				count--;
		} else if (fIncoming) {
			hyper_text *enclosure = (hyper_text *)malloc(sizeof(hyper_text));
			if (enclosure == NULL)
				return false;

			memset(enclosure, 0, sizeof(hyper_text));

			enclosure->type = TYPE_ENCLOSURE;
			enclosure->component = component;

			BString name;
			char fileName[B_FILE_NAME_LENGTH];
			strcpy(fileName, "untitled");
			if (BMailAttachment *attachment = dynamic_cast <BMailAttachment *> (component))
				attachment->FileName(fileName);

			BPath path(fileName);
			enclosure->name = strdup(path.Leaf());

			BMimeType type;
			component->MIMEType(&type);
			enclosure->content_type = strdup(type.Type());

			char typeDescription[B_MIME_TYPE_LENGTH];
			if (type.GetShortDescription(typeDescription) != B_OK)
				strcpy(typeDescription, type.Type() ? type.Type() : B_EMPTY_STRING);

			name = "\n<";
			name.Append(B_TRANSLATE_COMMENT("Enclosure: %name% (Type: %type%)",
				"Don't translate the variables %name% and %type%."));
			name.Append(">\n");
			name.ReplaceFirst("%name%", enclosure->name);
			name.ReplaceFirst("%type%", typeDescription);

			fView->GetSelection(&enclosure->text_start, &enclosure->text_end);
			enclosure->text_start++;
			enclosure->text_end += strlen(name.String()) - 1;

			Insert(name.String(), name.Length(), true);
			fEnclosures->AddItem(enclosure);
		}
//			default:
//			{
//				PlainTextBodyComponent *body = dynamic_cast<PlainTextBodyComponent *>(container->GetComponent(i));
//				const char *text;
//				if (body && (text = body->Text()) != NULL)
//					Insert(text, strlen(text), false);
//			}
	}
	return count > 0;
}


bool
TTextView::Reader::Process(const char *data, int32 data_len, bool isHeader)
{
	char line[522];
	int32 count = 0;

	for (int32 loop = 0; loop < data_len; loop++) {
		if (fView->fStopLoading)
			return false;

		if (fQuote && (!loop || (loop && data[loop - 1] == '\n'))) {
			strcpy(&line[count], QUOTE);
			count += strlen(QUOTE);
		}
		if (!fRaw && fIncoming && (loop < data_len - 7)) {
			size_t urlLength;
			BString url;
			uint8 type = CheckForURL(data + loop, urlLength, &url);

			if (type) {
				if (!Insert(line, count, false, isHeader))
					return false;
				count = 0;

				hyper_text *enclosure = (hyper_text *)malloc(sizeof(hyper_text));
				if (enclosure == NULL)
					return false;

				memset(enclosure, 0, sizeof(hyper_text));
				fView->GetSelection(&enclosure->text_start,
									&enclosure->text_end);
				enclosure->type = type;
				enclosure->name = strdup(url.String());
				if (enclosure->name == NULL) {
					free(enclosure);
					return false;
				}

				Insert(&data[loop], urlLength, true, isHeader);
				enclosure->text_end += urlLength;
				loop += urlLength - 1;

				fEnclosures->AddItem(enclosure);
				continue;
			}
		}
		if (!fRaw && fMime && data[loop] == '=') {
			if ((loop) && (loop < data_len - 1) && (data[loop + 1] == '\r'))
				loop += 2;
			else
				line[count++] = data[loop];
		} else if (data[loop] != '\r')
			line[count++] = data[loop];

		if (count > 511 || (count && loop == data_len - 1)) {
			if (!Insert(line, count, false, isHeader))
				return false;
			count = 0;
		}
	}
	return true;
}


bool
TTextView::Reader::Insert(const char *line, int32 count, bool isHyperLink,
	bool isHeader)
{
	if (!count)
		return true;

	BFont font(fView->Font());
	TextRunArray style(count / 8 + 8);

	if (fView->fColoredQuotes && !isHeader && !isHyperLink) {
		FillInQuoteTextRuns(fView, &fQuoteContext, line, count, font,
			&style.Array(), style.MaxEntries());
	} else {
		text_run_array &array = style.Array();
		array.count = 1;
		array.runs[0].offset = 0;
		if (isHeader) {
			array.runs[0].color = isHyperLink ? kHyperLinkColor : kHeaderColor;
			font.SetSize(font.Size() * 0.9);
		} else {
			array.runs[0].color = isHyperLink
				? kHyperLinkColor : kNormalTextColor;
		}
		array.runs[0].font = font;
	}

	if (!fView->Window()->Lock())
		return false;

	fView->Insert(fView->TextLength(), line, count, &style.Array());

	fView->Window()->Unlock();
	return true;
}


status_t
TTextView::Reader::Run(void *_this)
{
	Reader *reader = (Reader *)_this;
	TTextView *view = reader->fView;
	char *msg = NULL;
	off_t size = 0;
	int32 len = 0;

	if (!reader->Lock())
		return B_INTERRUPTED;

	BFile *file = dynamic_cast<BFile *>(reader->fMail->Data());
	if (file != NULL) {
		len = header_len(file);

		if (reader->fHeader)
			size = len;
		if (reader->fRaw || !reader->fMime)
			file->GetSize(&size);

		if (size != 0 && (msg = (char *)malloc(size)) == NULL)
			goto done;
		file->Seek(0, 0);

		if (msg)
			size = file->Read(msg, size);
	}

	// show the header?
	if (reader->fHeader && len) {
		// strip all headers except "From", "To", "Reply-To", "Subject", and "Date"
	 	if (reader->fStripHeader) {
		 	const char *header = msg;
		 	char *buffer = NULL;

			while (strncmp(header, "\r\n", 2)) {
				const char *eol = header;
				while ((eol = strstr(eol, "\r\n")) != NULL && isspace(eol[2]))
					eol += 2;
				if (eol == NULL)
					break;

				eol += 2;	// CR+LF belong to the line
				size_t length = eol - header;

		 		buffer = (char *)realloc(buffer, length + 1);
		 		if (buffer == NULL)
		 			goto done;

		 		memcpy(buffer, header, length);

				length = rfc2047_to_utf8(&buffer, &length, length);

		 		if (!strncasecmp(header, "Reply-To: ", 10)
		 			|| !strncasecmp(header, "To: ", 4)
		 			|| !strncasecmp(header, "From: ", 6)
		 			|| !strncasecmp(header, "Subject: ", 8)
		 			|| !strncasecmp(header, "Date: ", 6))
		 			reader->Process(buffer, length, true);

		 		header = eol;
	 		}
		 	free(buffer);
	 		reader->Process("\r\n", 2, true);
	 	}
	 	else if (!reader->Process(msg, len, true))
			goto done;
	}

	if (reader->fRaw) {
		if (!reader->Process((const char *)msg + len, size - len))
			goto done;
	} else {
		//reader->fFile->Seek(0, 0);
		//BEmailMessage *mail = new BEmailMessage(reader->fFile);
		BEmailMessage *mail = reader->fMail;

		// at first, insert the mail body
		BTextMailComponent *body = NULL;
		if (mail->BodyText() && !view->fStopLoading) {
			char *bodyText = const_cast<char *>(mail->BodyText());
			int32 bodyLength = strlen(bodyText);
			body = mail->Body();
			bool isHTML = false;

			BMimeType type;
			if (body->MIMEType(&type) == B_OK && type == "text/html") {
				// strip out HTML tags
				char *t = bodyText, *a, *end = bodyText + bodyLength;
				bodyText = (char *)malloc(bodyLength + 1);
				isHTML = true;

				// TODO: is it correct to assume that the text is in Latin-1?
				//		because if it isn't, the code below won't work correctly...

				for (a = bodyText; t < end; t++) {
					int32 c = *t;

					// compact spaces
					bool space = false;
					while (c && (c == ' ' || c == '\t')) {
						c = *(++t);
						space = true;
					}
					if (space) {
						c = ' ';
						t--;
					} else if (FilterHTMLTag(c, &t, end))	// the tag filter
						continue;

					Unicode2UTF8(c, &a);
				}

				*a = 0;
				bodyLength = strlen(bodyText);
				body = NULL;	// to add the HTML text as enclosure
			}
			if (!reader->Process(bodyText, bodyLength))
				goto done;

			if (isHTML)
				free(bodyText);
		}

		if (!reader->ParseMail(mail, body))
			goto done;

		//reader->fView->fMail = mail;
	}

	if (!view->fStopLoading && view->Window()->Lock()) {
		view->Select(0, 0);
		view->MakeSelectable(true);
		if (!reader->fIncoming)
			view->MakeEditable(true);

		view->Window()->Unlock();
	}

done:
	reader->Unlock();

	delete reader;
	free(msg);

	return B_NO_ERROR;
}


status_t
TTextView::Reader::Unlock()
{
	return release_sem(fStopSem);
}


bool
TTextView::Reader::Lock()
{
	if (acquire_sem_etc(fStopSem, 1, B_TIMEOUT, 0) != B_NO_ERROR)
		return false;

	return true;
}


//====================================================================
//	#pragma mark -


TSavePanel::TSavePanel(hyper_text *enclosure, TTextView *view)
	: BFilePanel(B_SAVE_PANEL)
{
	fEnclosure = enclosure;
	fView = view;
	if (enclosure->name)
		SetSaveText(enclosure->name);
}


void
TSavePanel::SendMessage(const BMessenger * /* messenger */, BMessage *msg)
{
	const char	*name = NULL;
	BMessage	save(M_SAVE);
	entry_ref	ref;

	if ((!msg->FindRef("directory", &ref)) && (!msg->FindString("name", &name))) {
		save.AddPointer("enclosure", fEnclosure);
		save.AddString("name", name);
		save.AddRef("directory", &ref);
		fView->Window()->PostMessage(&save, fView);
	}
}


void
TSavePanel::SetEnclosure(hyper_text *enclosure)
{
	fEnclosure = enclosure;
	if (enclosure->name)
		SetSaveText(enclosure->name);
	else
		SetSaveText("");

	if (!IsShowing())
		Show();
	Window()->Activate();
}


//--------------------------------------------------------------------
//	#pragma mark -


void
TTextView::InsertText(const char *insertText, int32 length, int32 offset,
	const text_run_array *runs)
{
	ContentChanged();

	// Undo function

	int32 cursorPos, dummy;
	GetSelection(&cursorPos, &dummy);

	if (fInputMethodUndoState.active) {
		// IMアクティブ時は、一旦別のバッファへ記憶
		fInputMethodUndoBuffer.AddUndo(insertText, length, offset, K_INSERTED, cursorPos);
		fInputMethodUndoState.replace = false;
	} else {
		if (fUndoState.replaced) {
			fUndoBuffer.AddUndo(insertText, length, offset, K_REPLACED, cursorPos);
		} else {
			if (length == 1 && insertText[0] == 0x0a)
				fUndoBuffer.MakeNewUndoItem();

			fUndoBuffer.AddUndo(insertText, length, offset, K_INSERTED, cursorPos);

			if (length == 1 && insertText[0] == 0x0a)
				fUndoBuffer.MakeNewUndoItem();
		}
	}

	fUndoState.replaced = false;
	fUndoState.deleted = false;

	struct text_runs : text_run_array { text_run _runs[1]; } style;
	if (runs == NULL && IsEditable()) {
		style.count = 1;
		style.runs[0].offset = 0;
		style.runs[0].font = fFont;
		style.runs[0].color = kNormalTextColor;
		runs = &style;
	}

	BTextView::InsertText(insertText, length, offset, runs);

	if (fSpellCheck && IsEditable())
	{
		UpdateSpellMarks(offset, length);

		rgb_color color;
		GetFontAndColor(offset - 1, NULL, &color);
		const char *text = Text();

		if (length > 1
			|| isalpha(text[offset + 1])
			|| (!isalpha(text[offset]) && text[offset] != '\'')
			|| (color.red == kSpellTextColor.red
				&& color.green == kSpellTextColor.green
				&& color.blue == kSpellTextColor.blue))
		{
			int32 start, end;
			FindSpellBoundry(length, offset, &start, &end);

			DSPELL(printf("Offset %ld, start %ld, end %ld\n", offset, start, end));
			DSPELL(printf("\t\"%10.10s...\"\n", text + start));

			CheckSpelling(start, end);
		}
	}
}


void
TTextView::DeleteText(int32 start, int32 finish)
{
	ContentChanged();

	// Undo function
	int32 cursorPos, dummy;
	GetSelection(&cursorPos, &dummy);
	if (fInputMethodUndoState.active) {
		if (fInputMethodUndoState.replace) {
			fUndoBuffer.AddUndo(&Text()[start], finish - start, start, K_DELETED, cursorPos);
			fInputMethodUndoState.replace = false;
		} else {
			fInputMethodUndoBuffer.AddUndo(&Text()[start], finish - start, start,
				K_DELETED, cursorPos);
		}
	} else
		fUndoBuffer.AddUndo(&Text()[start], finish - start, start, K_DELETED, cursorPos);

	fUndoState.deleted = true;
	fUndoState.replaced = true;

	BTextView::DeleteText(start, finish);
	if (fSpellCheck && IsEditable()) {
		UpdateSpellMarks(start, start - finish);

		int32 s, e;
		FindSpellBoundry(1, start, &s, &e);
		CheckSpelling(s, e);
	}
}


void
TTextView::ContentChanged(void)
{
	BLooper *looper = Looper();
	if (looper == NULL)
		return;

	BMessage msg(FIELD_CHANGED);
	msg.AddInt32("bitmask", FIELD_BODY);
	msg.AddPointer("source", this);
	looper->PostMessage(&msg);
}


void
TTextView::CheckSpelling(int32 start, int32 end, int32 flags)
{
	const char 	*text = Text();
	const char 	*next, *endPtr, *word = NULL;
	int32 		wordLength = 0, wordOffset;
	int32		nextHighlight = start;
	BString 	testWord;
	bool		isCap = false;
	bool		isAlpha;
	bool		isApost;

	for (next = text + start, endPtr = text + end; next <= endPtr; next++) {
		//printf("next=%c\n", *next);
		// ToDo: this has to be refined to other languages...
		// Alpha signifies the start of a word
		isAlpha = isalpha(*next);
		isApost = (*next == '\'');
		if (!word && isAlpha) {
			//printf("Found word start\n");
			word = next;
			wordLength++;
			isCap = isupper(*word);
		} else if (word && (isAlpha || isApost) && !(isApost && !isalpha(next[1]))
					&& !(isCap && isApost && (next[1] == 's'))) {
			// Word continues check
			wordLength++;
			//printf("Word continues...\n");
		} else if (word) {
			// End of word reached

			//printf("Word End\n");
			// Don't check single characters
			if (wordLength > 1) {
				bool isUpper = true;

				// Look for all uppercase
				for (int32 i = 0; i < wordLength; i++) {
					if (word[i] == '\'')
						break;

					if (islower(word[i])) {
						isUpper = false;
						break;
					}
				}

				// Don't check all uppercase words
				if (!isUpper) {
					bool foundMatch = false;
					wordOffset = word - text;
					testWord.SetTo(word, wordLength);

					testWord = testWord.ToLower();
					DSPELL(printf("Testing: \"%s\"\n", testWord.String()));

					int32 key = -1;
					if (gDictCount)
						key = gExactWords[0]->GetKey(testWord.String());

					// Search all dictionaries
					for (int32 i = 0; i < gDictCount; i++) {
						if (gExactWords[i]->Lookup(key) >= 0) {
							foundMatch = true;
							break;
						}
					}

					if (!foundMatch) {
						if (flags & S_CLEAR_ERRORS)
							RemoveSpellMark(nextHighlight, wordOffset);

						if (flags & S_SHOW_ERRORS)
							AddSpellMark(wordOffset, wordOffset + wordLength);
					} else if (flags & S_CLEAR_ERRORS)
						RemoveSpellMark(nextHighlight, wordOffset + wordLength);

					nextHighlight = wordOffset + wordLength;
				}
			}
			// Reset state to looking for word
			word = NULL;
			wordLength = 0;
		}
	}

	if (nextHighlight <= end
		&& (flags & S_CLEAR_ERRORS) != 0
		&& nextHighlight < TextLength())
		SetFontAndColor(nextHighlight, end, NULL, B_FONT_ALL, &kNormalTextColor);
}


void
TTextView::FindSpellBoundry(int32 length, int32 offset, int32 *_start, int32 *_end)
{
	int32 start, end, textLength;
	const char *text = Text();
	textLength = TextLength();

	for (start = offset - 1; start >= 0
		&& (isalpha(text[start]) || text[start] == '\''); start--) {}

	start++;

	for (end = offset + length; end < textLength
		&& (isalpha(text[end]) || text[end] == '\''); end++) {}

	*_start = start;
	*_end = end;
}


TTextView::spell_mark *
TTextView::FindSpellMark(int32 start, int32 end, spell_mark **_previousMark)
{
	spell_mark *lastMark = NULL;

	for (spell_mark *spellMark = fFirstSpellMark; spellMark; spellMark = spellMark->next) {
		if (spellMark->start < end && spellMark->end > start) {
			if (_previousMark)
				*_previousMark = lastMark;
			return spellMark;
		}

		lastMark = spellMark;
	}
	return NULL;
}


void
TTextView::UpdateSpellMarks(int32 offset, int32 length)
{
	DSPELL(printf("UpdateSpellMarks: offset = %ld, length = %ld\n", offset, length));

	spell_mark *spellMark;
	for (spellMark = fFirstSpellMark; spellMark; spellMark = spellMark->next) {
		DSPELL(printf("\tfound: %ld - %ld\n", spellMark->start, spellMark->end));

		if (spellMark->end < offset)
			continue;

		if (spellMark->start > offset)
			spellMark->start += length;

		spellMark->end += length;

		DSPELL(printf("\t-> reset: %ld - %ld\n", spellMark->start, spellMark->end));
	}
}


status_t
TTextView::AddSpellMark(int32 start, int32 end)
{
	DSPELL(printf("AddSpellMark: start = %ld, end = %ld\n", start, end));

	// check if there is already a mark for this passage
	spell_mark *spellMark = FindSpellMark(start, end);
	if (spellMark) {
		if (spellMark->start == start && spellMark->end == end) {
			DSPELL(printf("\tfound one\n"));
			return B_OK;
		}

		DSPELL(printf("\tremove old one\n"));
		RemoveSpellMark(start, end);
	}

	spellMark = (spell_mark *)malloc(sizeof(spell_mark));
	if (spellMark == NULL)
		return B_NO_MEMORY;

	spellMark->start = start;
	spellMark->end = end;
	spellMark->style = RunArray(start, end);

	// set the spell marks appearance
	BFont font(fFont);
	font.SetFace(B_BOLD_FACE | B_ITALIC_FACE);
	SetFontAndColor(start, end, &font, B_FONT_ALL, &kSpellTextColor);

	// add it to the queue
	spellMark->next = fFirstSpellMark;
	fFirstSpellMark = spellMark;

	return B_OK;
}


bool
TTextView::RemoveSpellMark(int32 start, int32 end)
{
	DSPELL(printf("RemoveSpellMark: start = %ld, end = %ld\n", start, end));

	// find spell mark
	spell_mark *lastMark = NULL;
	spell_mark *spellMark = FindSpellMark(start, end, &lastMark);
	if (spellMark == NULL) {
		DSPELL(printf("\tnot found!\n"));
		return false;
	}

	DSPELL(printf("\tfound: %ld - %ld\n", spellMark->start, spellMark->end));

	// dequeue the spell mark
	if (lastMark)
		lastMark->next = spellMark->next;
	else
		fFirstSpellMark = spellMark->next;

	if (spellMark->start < start)
		start = spellMark->start;
	if (spellMark->end > end)
		end = spellMark->end;

	// reset old text run array
	SetRunArray(start, end, spellMark->style);

	free(spellMark->style);
	free(spellMark);

	return true;
}


void
TTextView::RemoveSpellMarks()
{
	spell_mark *spellMark, *nextMark;

	for (spellMark = fFirstSpellMark; spellMark; spellMark = nextMark) {
		nextMark = spellMark->next;

		// reset old text run array
		SetRunArray(spellMark->start, spellMark->end, spellMark->style);

		free(spellMark->style);
		free(spellMark);
	}

	fFirstSpellMark = NULL;
}


void
TTextView::EnableSpellCheck(bool enable)
{
	if (fSpellCheck == enable)
		return;

	fSpellCheck = enable;
	int32 textLength = TextLength();
	if (fSpellCheck) {
		// work-around for a bug in the BTextView class
		// which causes lots of flicker
		int32 start, end;
		GetSelection(&start, &end);
		if (start != end)
			Select(start, start);

		CheckSpelling(0, textLength);

		if (start != end)
			Select(start, end);
	}
	else
		RemoveSpellMarks();
}


void
TTextView::WindowActivated(bool flag)
{
	if (!flag) {
		// WindowActivated(false) は、IM も Inactive になり、そのまま確定される。
		// しかしこの場合、input_server が B_INPUT_METHOD_EVENT(B_INPUT_METHOD_STOPPED)
		// を送ってこないまま矛盾してしまうので、やむを得ずここでつじつまあわせ処理している。
		// OpenBeOSで修正されることを願って暫定処置としている。
		fInputMethodUndoState.active = false;
		// fInputMethodUndoBufferに溜まっている最後のデータがK_INSERTEDなら（確定）正規のバッファへ追加
		if (fInputMethodUndoBuffer.CountItems() > 0) {
			KUndoItem *item = fInputMethodUndoBuffer.ItemAt(fInputMethodUndoBuffer.CountItems() - 1);
			if (item->History == K_INSERTED) {
				fUndoBuffer.MakeNewUndoItem();
				fUndoBuffer.AddUndo(item->RedoText, item->Length, item->Offset,
					item->History, item->CursorPos);
				fUndoBuffer.MakeNewUndoItem();
			}
			fInputMethodUndoBuffer.MakeEmpty();
		}
	}
	BTextView::WindowActivated(flag);
}


void
TTextView::AddQuote(int32 start, int32 finish)
{
	BRect rect = Bounds();

	int32 lineStart;
	GoToLine(CurrentLine());
	GetSelection(&lineStart, &lineStart);

	// make sure that we're changing the whole last line, too
	int32 lineEnd = finish > lineStart ? finish - 1 : finish;
	{
		const char *text = Text();
		while (text[lineEnd] && text[lineEnd] != '\n')
			lineEnd++;
	}
	Select(lineStart, lineEnd);

	int32 textLength = lineEnd - lineStart;
	char *text = (char *)malloc(textLength + 1);
	if (text == NULL)
		return;

	GetText(lineStart, textLength, text);

	int32 quoteLength = strlen(QUOTE);
	int32 targetLength = 0;
	char *target = NULL;
	int32 lastLine = 0;

	for (int32 index = 0; index < textLength; index++) {
		if (text[index] == '\n' || index == textLength - 1) {
			// add quote to this line
			int32 lineLength = index - lastLine + 1;

			target = (char *)realloc(target, targetLength + lineLength + quoteLength);
			if (target == NULL) {
				// free the old buffer?
				free(text);
				return;
			}

			// copy the quote sign
			memcpy(&target[targetLength], QUOTE, quoteLength);
			targetLength += quoteLength;

			// copy the rest of the line
			memcpy(&target[targetLength], &text[lastLine], lineLength);
			targetLength += lineLength;

			lastLine = index + 1;
		}
	}

	// replace with quoted text
	free(text);
	Delete();

	if (fColoredQuotes) {
		const BFont *font = Font();
		TextRunArray style(targetLength / 8 + 8);

		FillInQuoteTextRuns(NULL, NULL, target, targetLength, font,
			&style.Array(), style.MaxEntries());
		Insert(target, targetLength, &style.Array());
	} else
		Insert(target, targetLength);

	free(target);

	// redo the old selection (compute the new start if necessary)
	Select(start + quoteLength, finish + (targetLength - textLength));

	ScrollTo(rect.LeftTop());
}


void
TTextView::RemoveQuote(int32 start, int32 finish)
{
	BRect rect = Bounds();

	GoToLine(CurrentLine());
	int32 lineStart;
	GetSelection(&lineStart, &lineStart);

	// make sure that we're changing the whole last line, too
	int32 lineEnd = finish > lineStart ? finish - 1 : finish;
	const char *text = Text();
	while (text[lineEnd] && text[lineEnd] != '\n')
		lineEnd++;

	Select(lineStart, lineEnd);

	int32 length = lineEnd - lineStart;
	char *target = (char *)malloc(length + 1);
	if (target == NULL)
		return;

	int32 quoteLength = strlen(QUOTE);
	int32 removed = 0;
	text += lineStart;

	for (int32 index = 0; index < length;) {
		// find out the length of the current line
		int32 lineLength = 0;
		while (index + lineLength < length && text[lineLength] != '\n')
			lineLength++;

		// include the newline to be part of this line
		if (text[lineLength] == '\n' && index + lineLength + 1 < length)
			lineLength++;

		if (!strncmp(text, QUOTE, quoteLength)) {
			// remove quote
			length -= quoteLength;
			removed += quoteLength;

			lineLength -= quoteLength;
			text += quoteLength;
		}

		if (lineLength == 0) {
			target[index] = '\0';
			break;
		}

		memcpy(&target[index], text, lineLength);

		text += lineLength;
		index += lineLength;
	}

	if (removed) {
		Delete();

		if (fColoredQuotes) {
			const BFont *font = Font();
			TextRunArray style(length / 8 + 8);

			FillInQuoteTextRuns(NULL, NULL, target, length, font,
				&style.Array(), style.MaxEntries());
			Insert(target, length, &style.Array());
		} else
			Insert(target, length);

		// redo old selection
		bool noSelection = start == finish;

		if (start > lineStart + quoteLength)
			start -= quoteLength;
		else
			start = lineStart;

		if (noSelection)
			finish = start;
		else
			finish -= removed;
	}

	free(target);

	Select(start, finish);
	ScrollTo(rect.LeftTop());
}


void
TTextView::Undo(BClipboard */*clipboard*/)
{
	if (fInputMethodUndoState.active)
		return;

	int32 length, offset, cursorPos;
	undo_type history;
	char *text;
	status_t status;

	status = fUndoBuffer.Undo(&text, &length, &offset, &history, &cursorPos);
	if (status == B_OK) {
		fUndoBuffer.Off();

		switch (history) {
			case K_INSERTED:
				BTextView::Delete(offset, offset + length);
				Select(offset, offset);
				break;

			case K_DELETED:
				BTextView::Insert(offset, text, length);
				Select(offset, offset + length);
				break;

			case K_REPLACED:
				BTextView::Delete(offset, offset + length);
				status = fUndoBuffer.Undo(&text, &length, &offset, &history, &cursorPos);
				if (status == B_OK && history == K_DELETED) {
					BTextView::Insert(offset, text, length);
					Select(offset, offset + length);
				} else {
					::beep();
					(new BAlert("",
						B_TRANSLATE("Inconsistency occurred in the undo/redo "
							"buffer."),
						B_TRANSLATE("OK")))->Go();
				}
				break;
		}
		ScrollToSelection();
		ContentChanged();
		fUndoBuffer.On();
	}
}


void
TTextView::Redo()
{
	if (fInputMethodUndoState.active)
		return;

	int32 length, offset, cursorPos;
	undo_type history;
	char *text;
	status_t status;
	bool replaced;

	status = fUndoBuffer.Redo(&text, &length, &offset, &history, &cursorPos, &replaced);
	if (status == B_OK) {
		fUndoBuffer.Off();

		switch (history) {
			case K_INSERTED:
				BTextView::Insert(offset, text, length);
				Select(offset, offset + length);
				break;

			case K_DELETED:
				BTextView::Delete(offset, offset + length);
				if (replaced) {
					fUndoBuffer.Redo(&text, &length, &offset, &history, &cursorPos, &replaced);
					BTextView::Insert(offset, text, length);
				}
				Select(offset, offset + length);
				break;

			case K_REPLACED:
				::beep();
				(new BAlert("",
					B_TRANSLATE("Inconsistency occurred in the undo/redo "
						"buffer."),
					B_TRANSLATE("OK")))->Go();
				break;
		}
		ScrollToSelection();
		ContentChanged();
		fUndoBuffer.On();
	}
}

