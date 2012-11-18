/*
 * Copyright 2001-2010, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai.
 * Distributed under the terms of the MIT license.
 */


//! Escape sequence parse and character encoding.


#include "TermParse.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <Autolock.h>
#include <Beep.h>
#include <Catalog.h>
#include <Locale.h>
#include <Message.h>
#include <UTF8.h>

#include "TermConst.h"
#include "TerminalBuffer.h"
#include "VTparse.h"


extern int gUTF8GroundTable[];		/* UTF8 Ground table */
extern int gCS96GroundTable[];		/* CS96 Ground table */
extern int gISO8859GroundTable[];	/* ISO8859 & EUC Ground table */
extern int gWinCPGroundTable[];		/* Windows cp1252, cp1251, koi-8r */
extern int gSJISGroundTable[];		/* Shift-JIS Ground table */

extern int gEscTable[];				/* ESC */
extern int gCsiTable[];				/* ESC [ */
extern int gDecTable[];				/* ESC [ ? */
extern int gScrTable[];				/* ESC # */
extern int gIgnoreTable[];			/* ignore table */
extern int gIesTable[];				/* ignore ESC table */
extern int gEscIgnoreTable[];		/* ESC ignore table */
extern int gMbcsTable[];			/* ESC $ */

extern int gLineDrawTable[];		/* ESC ( 0 */


#define DEFAULT -1
#define NPARAM 10		// Max parameters


//! Get char from pty reader buffer.
inline uchar
TermParse::_NextParseChar()
{
	if (fParserBufferOffset >= fParserBufferSize) {
		// parser buffer empty
		status_t error = _ReadParserBuffer();
		if (error != B_OK)
			throw error;
	}

	return fParserBuffer[fParserBufferOffset++];
}


TermParse::TermParse(int fd)
	:
	fFd(fd),
	fAttr(FORECOLORED(7)),
	fSavedAttr(FORECOLORED(7)),
	fParseThread(-1),
	fReaderThread(-1),
	fReaderSem(-1),
	fReaderLocker(-1),
	fBufferPosition(0),
	fReadBufferSize(0),
	fParserBufferSize(0),
	fParserBufferOffset(0),
	fBuffer(NULL),
	fQuitting(true)
{
	memset(fReadBuffer, 0, READ_BUF_SIZE);
	memset(fParserBuffer, 0, ESC_PARSER_BUFFER_SIZE);
}


TermParse::~TermParse()
{
	StopThreads();
}


status_t
TermParse::StartThreads(TerminalBuffer *buffer)
{
	if (fBuffer != NULL)
		return B_ERROR;

	fQuitting = false;
	fBuffer = buffer;

	status_t status = _InitPtyReader();
	if (status < B_OK) {
		fBuffer = NULL;
		return status;
	}

	status = _InitTermParse();
	if (status < B_OK) {
		_StopPtyReader();
		fBuffer = NULL;
		return status;
	}

	return B_OK;
}


status_t
TermParse::StopThreads()
{
	if (fBuffer == NULL)
		return B_ERROR;

	fQuitting = true;

	_StopPtyReader();
	_StopTermParse();

	fBuffer = NULL;

	return B_OK;
}


//! Initialize and spawn EscParse thread.
status_t
TermParse::_InitTermParse()
{
	if (fParseThread >= 0)
		return B_ERROR; // we might want to return B_OK instead ?

	fParseThread = spawn_thread(_escparse_thread, "EscParse",
		B_DISPLAY_PRIORITY, this);

	if (fParseThread < 0)
		return fParseThread;

	resume_thread(fParseThread);

	return B_OK;
}


//! Initialize and spawn PtyReader thread.
status_t
TermParse::_InitPtyReader()
{
	if (fReaderThread >= 0)
		return B_ERROR; // same as above

	fReaderSem = create_sem(0, "pty_reader_sem");
	if (fReaderSem < 0)
		return fReaderSem;

	fReaderLocker = create_sem(0, "pty_locker_sem");
	if (fReaderLocker < 0) {
		delete_sem(fReaderSem);
		fReaderSem = -1;
		return fReaderLocker;
	}

	fReaderThread = spawn_thread(_ptyreader_thread, "PtyReader",
		B_NORMAL_PRIORITY, this);
  	if (fReaderThread < 0) {
		delete_sem(fReaderSem);
		fReaderSem = -1;
		delete_sem(fReaderLocker);
		fReaderLocker = -1;
		return fReaderThread;
	}

	resume_thread(fReaderThread);

	return B_OK;
}


void
TermParse::_StopTermParse()
{
	if (fParseThread >= 0) {
		status_t dummy;
		wait_for_thread(fParseThread, &dummy);
		fParseThread = -1;
	}
}


void
TermParse::_StopPtyReader()
{
	if (fReaderSem >= 0) {
		delete_sem(fReaderSem);
		fReaderSem = -1;
	}
	if (fReaderLocker >= 0) {
		delete_sem(fReaderLocker);
		fReaderLocker = -1;
	}

	if (fReaderThread >= 0) {
		suspend_thread(fReaderThread);

		status_t status;
		wait_for_thread(fReaderThread, &status);

		fReaderThread = -1;
	}
}


//! Get data from pty device.
int32
TermParse::PtyReader()
{
	int32 bufferSize = 0;
	int32 readPos = 0;
	while (!fQuitting) {
		// If Pty Buffer nearly full, snooze this thread, and continue.
		while (READ_BUF_SIZE - bufferSize < MIN_PTY_BUFFER_SPACE) {
			status_t status;
			do {
				status = acquire_sem(fReaderLocker);
			} while (status == B_INTERRUPTED);
			if (status < B_OK)
				return status;

			bufferSize = fReadBufferSize;
		}

		// Read PTY
		uchar buf[READ_BUF_SIZE];
		ssize_t nread = read(fFd, buf, READ_BUF_SIZE - bufferSize);
		if (nread <= 0) {
			fBuffer->NotifyQuit(errno);
			return B_OK;
		}

		// Copy read string to PtyBuffer.

		int32 left = READ_BUF_SIZE - readPos;

		if (nread >= left) {
			memcpy(fReadBuffer + readPos, buf, left);
			memcpy(fReadBuffer, buf + left, nread - left);
		} else
			memcpy(fReadBuffer + readPos, buf, nread);

		bufferSize = atomic_add(&fReadBufferSize, nread);
		if (bufferSize == 0)
			release_sem(fReaderSem);

		bufferSize += nread;
		readPos = (readPos + nread) % READ_BUF_SIZE;
	}

	return B_OK;
}


void
TermParse::DumpState(int *groundtable, int *parsestate, uchar c)
{
	static const struct {
		int *p;
		const char *name;
	} tables[] = {
#define T(t) \
	{ t, #t }
		T(gUTF8GroundTable),
		T(gCS96GroundTable),
		T(gISO8859GroundTable),
		T(gWinCPGroundTable),
		T(gSJISGroundTable),
		T(gEscTable),
		T(gCsiTable),
		T(gDecTable),
		T(gScrTable),
		T(gIgnoreTable),
		T(gIesTable),
		T(gEscIgnoreTable),
		T(gMbcsTable),
		{ NULL, NULL }
	};
	int i;
	fprintf(stderr, "groundtable: ");
	for (i = 0; tables[i].p; i++) {
		if (tables[i].p == groundtable)
			fprintf(stderr, "%s\t", tables[i].name);
	}
	fprintf(stderr, "parsestate: ");
	for (i = 0; tables[i].p; i++) {
		if (tables[i].p == parsestate)
			fprintf(stderr, "%s\t", tables[i].name);
	}
	fprintf(stderr, "char: 0x%02x (%d)\n", c, c);
}


int *
TermParse::_GuessGroundTable(int encoding)
{
	switch (encoding) {
		case B_ISO1_CONVERSION:
		case B_ISO2_CONVERSION:
		case B_ISO3_CONVERSION:
		case B_ISO4_CONVERSION:
		case B_ISO5_CONVERSION:
		case B_ISO6_CONVERSION:
		case B_ISO7_CONVERSION:
		case B_ISO8_CONVERSION:
		case B_ISO9_CONVERSION:
		case B_ISO10_CONVERSION:
		case B_ISO13_CONVERSION:
		case B_ISO14_CONVERSION:
		case B_ISO15_CONVERSION:
		case B_EUC_CONVERSION:
		case B_EUC_KR_CONVERSION:
		case B_JIS_CONVERSION:
		case B_GBK_CONVERSION:
		case B_BIG5_CONVERSION:
			return gISO8859GroundTable;

		case B_KOI8R_CONVERSION:
		case B_MS_WINDOWS_1251_CONVERSION:
		case B_MS_WINDOWS_CONVERSION:
		case B_MAC_ROMAN_CONVERSION:
		case B_MS_DOS_866_CONVERSION:
		case B_MS_DOS_CONVERSION:
			return gWinCPGroundTable;

		case B_SJIS_CONVERSION:
			return gSJISGroundTable;

		case M_UTF8:
		default:
			break;
	}

	return gUTF8GroundTable;
}


int32
TermParse::EscParse()
{
	int top;
	int bottom;
//	int cs96 = 0;
	uchar curess = 0;

	char cbuf[4] = { 0 };
	char dstbuf[4] = { 0 };
	char *ptr;

	int currentEncoding = -1;

	int param[NPARAM];
	int nparam = 1;

	int row;
	int column;

	/* default encoding system is UTF8 */
	int *groundtable = gUTF8GroundTable;
	int *parsestate = gUTF8GroundTable;

	/* Handle switch between G0 and G1 character sets */
	int *alternateParseTable = gUTF8GroundTable;
	bool shifted_in = false;

	int32 srcLen = sizeof(cbuf);
	int32 dstLen = sizeof(dstbuf);
	int32 dummyState = 0;

	int width = 1;
	BAutolock locker(fBuffer);

	fAttr = fSavedAttr = FORECOLORED(7);

	while (!fQuitting) {
		try {
			uchar c = _NextParseChar();

			//DumpState(groundtable, parsestate, c);

			if (currentEncoding != fBuffer->Encoding()) {
				// Change coding, change parse table.
				groundtable = _GuessGroundTable(fBuffer->Encoding());
				parsestate = groundtable;
				currentEncoding = fBuffer->Encoding();
			}

	//debug_printf("TermParse: char: '%c' (%d), parse state: %d\n", c, c, parsestate[c]);
			switch (parsestate[c]) {
				case CASE_PRINT:
					fBuffer->InsertChar((char)c, fAttr);
					break;

				case CASE_PRINT_GR:
					/* case iso8859 gr character, or euc */
					ptr = cbuf;
					if (currentEncoding == B_EUC_CONVERSION
							|| currentEncoding == B_EUC_KR_CONVERSION
							|| currentEncoding == B_JIS_CONVERSION
							|| currentEncoding == B_GBK_CONVERSION
							|| currentEncoding == B_BIG5_CONVERSION) {
						switch (parsestate[curess]) {
							case CASE_SS2:		/* JIS X 0201 */
								width = 1;
								*ptr++ = curess;
								*ptr++ = c;
								*ptr = 0;
								curess = 0;
								break;

							case CASE_SS3:		/* JIS X 0212 */
								width = 1;
								*ptr++ = curess;
								*ptr++ = c;
								c = _NextParseChar();
								*ptr++ = c;
								*ptr = 0;
								curess = 0;
								break;

							default:		/* JIS X 0208 */
								width = 2;
								*ptr++ = c;
								c = _NextParseChar();
								*ptr++ = c;
								*ptr = 0;
								break;
						}
					} else {
						/* ISO-8859-1...10 and MacRoman */
						*ptr++ = c;
						*ptr = 0;
					}

					srcLen = strlen(cbuf);
					dstLen = sizeof(dstbuf);
					if (currentEncoding != B_JIS_CONVERSION) {
						convert_to_utf8(currentEncoding, cbuf, &srcLen,
								dstbuf, &dstLen, &dummyState, '?');
					} else {
						convert_to_utf8(B_EUC_CONVERSION, cbuf, &srcLen,
								dstbuf, &dstLen, &dummyState, '?');
					}

					fBuffer->InsertChar(dstbuf, dstLen, width, fAttr);
					break;

				case CASE_PRINT_CS96:
					cbuf[0] = c | 0x80;
					c = _NextParseChar();
					cbuf[1] = c | 0x80;
					cbuf[2] = 0;
					srcLen = 2;
					dstLen = sizeof(dstbuf);
					convert_to_utf8(B_EUC_CONVERSION, cbuf, &srcLen,
							dstbuf, &dstLen, &dummyState, '?');
					fBuffer->InsertChar(dstbuf, dstLen, fAttr);
					break;

				case CASE_PRINT_GRA:
					/* "Special characters and line drawing" enabled by \E(0 */
					switch (c) {
						case 'a':
							fBuffer->InsertChar("\xE2\x96\x92",3,fAttr);
							break;
						case 'j':
							fBuffer->InsertChar("\xE2\x94\x98",3,fAttr);
							break;
						case 'k':
							fBuffer->InsertChar("\xE2\x94\x90",3,fAttr);
							break;
						case 'l':
							fBuffer->InsertChar("\xE2\x94\x8C",3,fAttr);
							break;
						case 'm':
							fBuffer->InsertChar("\xE2\x94\x94",3,fAttr);
							break;
						case 'n':
							fBuffer->InsertChar("\xE2\x94\xBC",3,fAttr);
							break;
						case 'q':
							fBuffer->InsertChar("\xE2\x94\x80",3,fAttr);
							break;
						case 't':
							fBuffer->InsertChar("\xE2\x94\x9C",3,fAttr);
							break;
						case 'u':
							fBuffer->InsertChar("\xE2\x94\xA4",3,fAttr);
							break;
						case 'v':
							fBuffer->InsertChar("\xE2\x94\xB4",3,fAttr);
							break;
						case 'w':
							fBuffer->InsertChar("\xE2\x94\xAC",3,fAttr);
							break;
						case 'x':
							fBuffer->InsertChar("\xE2\x94\x82",3,fAttr);
							break;
						default:
							fBuffer->InsertChar((char)c, fAttr);
					}
					break;

				case CASE_LF:
					fBuffer->InsertLF();
					break;

				case CASE_CR:
					fBuffer->InsertCR(fAttr);
					break;

				case CASE_SJIS_KANA:
					cbuf[0] = c;
					cbuf[1] = '\0';
					srcLen = 1;
					dstLen = sizeof(dstbuf);
					convert_to_utf8(currentEncoding, cbuf, &srcLen,
							dstbuf, &dstLen, &dummyState, '?');
					fBuffer->InsertChar(dstbuf, dstLen, fAttr);
					break;

				case CASE_SJIS_INSTRING:
					cbuf[0] = c;
					c = _NextParseChar();
					cbuf[1] = c;
					cbuf[2] = '\0';
					srcLen = 2;
					dstLen = sizeof(dstbuf);
					convert_to_utf8(currentEncoding, cbuf, &srcLen,
							dstbuf, &dstLen, &dummyState, '?');
					fBuffer->InsertChar(dstbuf, dstLen, fAttr);
					break;

				case CASE_UTF8_2BYTE:
					cbuf[0] = c;
					c = _NextParseChar();
					if (groundtable[c] != CASE_UTF8_INSTRING)
						break;
					cbuf[1] = c;
					cbuf[2] = '\0';

					fBuffer->InsertChar(cbuf, 2, fAttr);
					break;

				case CASE_UTF8_3BYTE:
					cbuf[0] = c;
					c = _NextParseChar();
					if (groundtable[c] != CASE_UTF8_INSTRING)
						break;
					cbuf[1] = c;

					c = _NextParseChar();
					if (groundtable[c] != CASE_UTF8_INSTRING)
						break;
					cbuf[2] = c;
					cbuf[3] = '\0';
					fBuffer->InsertChar(cbuf, 3, fAttr);
					break;

				case CASE_MBCS:
					/* ESC $ */
					parsestate = gMbcsTable;
					break;

				case CASE_GSETS:
					/* ESC $ ? */
					parsestate = gCS96GroundTable;
					//		cs96 = 1;
					break;

				case CASE_SCS_STATE:
				{
					char page = _NextParseChar();

					int* newTable = _GuessGroundTable(currentEncoding);
					if (page == '0')
						newTable = gLineDrawTable;

					if (c == '(') {
						if (shifted_in)
							alternateParseTable = newTable;
						else
							groundtable = newTable;
					} else if (c == ')') {
						if (!shifted_in)
							alternateParseTable = newTable;
						else
							groundtable = newTable;
					}

					parsestate = groundtable;

					break;
				}

				case CASE_GROUND_STATE:
					/* exit ignore mode */
					parsestate = groundtable;
					break;

				case CASE_BELL:
					beep();
					break;

				case CASE_BS:
					fBuffer->MoveCursorLeft(1);
					break;

				case CASE_TAB:
					fBuffer->InsertTab(fAttr);
					break;

				case CASE_ESC:
					/* escape */
					parsestate = gEscTable;
					break;

				case CASE_IGNORE_STATE:
					/* Ies: ignore anything else */
					parsestate = gIgnoreTable;
					break;

				case CASE_IGNORE_ESC:
					/* Ign: escape */
					parsestate = gIesTable;
					break;

				case CASE_IGNORE:
					/* Ignore character */
					break;

				case CASE_SI:
					/* shift in (to G1 charset) */
					if (shifted_in == false) {
						int* tmp = alternateParseTable;
						alternateParseTable = parsestate;
						parsestate = tmp;
					}
					break;

				case CASE_SO:
					/* shift out (to G0 charset) */
					if (shifted_in == true) {
						int* tmp = alternateParseTable;
						alternateParseTable = parsestate;
						parsestate = tmp;
					}
					break;

				case CASE_SCR_STATE:	// ESC #
					/* enter scr state */
					parsestate = gScrTable;
					break;

				case CASE_ESC_IGNORE:
					/* unknown escape sequence */
					parsestate = gEscIgnoreTable;
					break;

				case CASE_ESC_DIGIT:	// ESC [ number
					/* digit in csi or dec mode */
					if ((row = param[nparam - 1]) == DEFAULT)
						row = 0;
					param[nparam - 1] = 10 * row + (c - '0');
					break;

				case CASE_ESC_SEMI:		// ESC ;
					/* semicolon in csi or dec mode */
					if (nparam < NPARAM)
						param[nparam++] = DEFAULT;
					break;

				case CASE_DEC_STATE:
					/* enter dec mode */
					parsestate = gDecTable;
					break;

				case CASE_ICH:		// ESC [@ insert charactor
					/* ICH */
					if ((row = param[0]) < 1)
						row = 1;
					fBuffer->InsertSpace(row);
					parsestate = groundtable;
					break;

				case CASE_CUU:		// ESC [A cursor up, up arrow key.
					/* CUU */
					if ((row = param[0]) < 1)
						row = 1;
					fBuffer->MoveCursorUp(row);
					parsestate = groundtable;
					break;

				case CASE_CUD:		// ESC [B cursor down, down arrow key.
					/* CUD */
					if ((row = param[0]) < 1)
						row = 1;
					fBuffer->MoveCursorDown(row);
					parsestate = groundtable;
					break;

				case CASE_CUF:		// ESC [C cursor forword
					/* CUF */
					if ((row = param[0]) < 1)
						row = 1;
					fBuffer->MoveCursorRight(row);
					parsestate = groundtable;
					break;

				case CASE_CUB:		// ESC [D cursor backword
					/* CUB */
					if ((row = param[0]) < 1)
						row = 1;
					fBuffer->MoveCursorLeft(row);
					parsestate = groundtable;
					break;

				case CASE_CUP:		// ESC [...H move cursor
					/* CUP | HVP */
					if ((row = param[0]) < 1)
						row = 1;
					if (nparam < 2 || (column = param[1]) < 1)
						column = 1;

					fBuffer->SetCursor(column - 1, row - 1 );
					parsestate = groundtable;
					break;

				case CASE_ED:		// ESC [ ...J clear screen
					/* ED */
					switch (param[0]) {
						case DEFAULT:
						case 0:
							fBuffer->EraseBelow();
							break;

						case 1:
							fBuffer->EraseAbove();
							break;

						case 2:
							fBuffer->EraseAll();
							break;
					}
					parsestate = groundtable;
					break;

				case CASE_EL:		// ESC [ ...K delete line
					/* EL */
					switch (param[0]) {
						case DEFAULT:
						case 0:
							fBuffer->DeleteColumns();
							break;

						case 1:
							fBuffer->EraseCharsFrom(0, fBuffer->Cursor().x + 1);
							break;

						case 2:
							fBuffer->DeleteColumnsFrom(0);
							break;
					}
					parsestate = groundtable;
					break;

				case CASE_IL:
					/* IL */
					if ((row = param[0]) < 1)
						row = 1;
					fBuffer->InsertLines(row);
					parsestate = groundtable;
					break;

				case CASE_DL:
					/* DL */
					if ((row = param[0]) < 1)
						row = 1;
					fBuffer->DeleteLines(row);
					parsestate = groundtable;
					break;

				case CASE_DCH:
					/* DCH */
					if ((row = param[0]) < 1)
						row = 1;
					fBuffer->DeleteChars(row);
					parsestate = groundtable;
					break;

				case CASE_SET:
					/* SET */
					if (param[0] == 4)
						fBuffer->SetInsertMode(MODE_INSERT);
					parsestate = groundtable;
					break;

				case CASE_RST:
					/* RST */
					if (param[0] == 4)
						fBuffer->SetInsertMode(MODE_OVER);
					parsestate = groundtable;
					break;

				case CASE_SGR:
				{
					/* SGR */
					for (row = 0; row < nparam; ++row) {
						switch (param[row]) {
							case DEFAULT:
							case 0: /* Reset attribute */
								fAttr = FORECOLORED(7);
								break;

							case 1: /* Bold     */
							case 5:
								fAttr |= BOLD;
								break;

							case 4:	/* Underline	*/
								fAttr |= UNDERLINE;
								break;

							case 7:	/* Inverse	*/
								fAttr |= INVERSE;
								break;

							case 22:	/* Not Bold	*/
								fAttr &= ~BOLD;
								break;

							case 24:	/* Not Underline	*/
								fAttr &= ~UNDERLINE;
								break;

							case 27:	/* Not Inverse	*/
								fAttr &= ~INVERSE;
								break;

							case 30:
							case 31:
							case 32:
							case 33:
							case 34:
							case 35:
							case 36:
							case 37:
								fAttr &= ~FORECOLOR;
								fAttr |= FORECOLORED(param[row] - 30);
								fAttr |= FORESET;
								break;

							case 38:
							{
								if (nparam != 3 || param[1] != 5)
									break;
								fAttr &= ~FORECOLOR;
								fAttr |= FORECOLORED(param[2]);
								fAttr |= FORESET;

								row = nparam; // force exit of the parsing

								break;
							}

							case 39:
								fAttr &= ~FORESET;
								break;

							case 40:
							case 41:
							case 42:
							case 43:
							case 44:
							case 45:
							case 46:
							case 47:
								fAttr &= ~BACKCOLOR;
								fAttr |= BACKCOLORED(param[row] - 40);
								fAttr |= BACKSET;
								break;

							case 48:
							{
								if (nparam != 3 || param[1] != 5)
									break;
								fAttr &= ~BACKCOLOR;
								fAttr |= BACKCOLORED(param[2]);
								fAttr |= BACKSET;

								row = nparam; // force exit of the parsing

								break;
							}

							case 49:
								fAttr &= ~BACKSET;
								break;
						}
					}
					parsestate = groundtable;
					break;
				}

				case CASE_CPR:
				// Q & D hack by Y.Hayakawa (hida@sawada.riec.tohoku.ac.jp)
				// 21-JUL-99
				_DeviceStatusReport(param[0]);
				parsestate = groundtable;
				break;

				case CASE_DA1:
				// DA - report device attributes
				if (param[0] < 1) {
					// claim to be a VT102
					write(fFd, "\033[?6c", 5);
				}
				parsestate = groundtable;
				break;

				case CASE_DECSTBM:
				/* DECSTBM - set scrolling region */

				if ((top = param[0]) < 1)
					top = 1;

				if (nparam < 2)
					bottom = fBuffer->Height();
				else
					bottom = param[1];

				top--;
					bottom--;

					if (bottom > top)
						fBuffer->SetScrollRegion(top, bottom);

					parsestate = groundtable;
					break;

				case CASE_DECREQTPARM:
					// DEXREQTPARM - request terminal parameters
					_DecReqTermParms(param[0]);
					parsestate = groundtable;
					break;

				case CASE_DECSET:
					/* DECSET */
					for (int i = 0; i < nparam; i++)
						_DecPrivateModeSet(param[i]);
					parsestate = groundtable;
					break;

				case CASE_DECRST:
					/* DECRST */
					for (int i = 0; i < nparam; i++)
						_DecPrivateModeReset(param[i]);
					parsestate = groundtable;
					break;

				case CASE_DECALN:
					/* DECALN */
					fBuffer->FillScreen(UTF8Char('E'), 1, 0);
					parsestate = groundtable;
					break;

					//	case CASE_GSETS:
					//		screen->gsets[scstype] = GSET(c) | cs96;
					//		parsestate = groundtable;
					//		break;

				case CASE_DECSC:
					/* DECSC */
					_DecSaveCursor();
					parsestate = groundtable;
					break;

				case CASE_DECRC:
					/* DECRC */
					_DecRestoreCursor();
					parsestate = groundtable;
					break;

				case CASE_HTS:
					/* HTS */
					fBuffer->SetTabStop(fBuffer->Cursor().x);
					parsestate = groundtable;
					break;

				case CASE_TBC:
					/* TBC */
					if (param[0] < 1)
						fBuffer->ClearTabStop(fBuffer->Cursor().x);
					else if (param[0] == 3)
						fBuffer->ClearAllTabStops();
					parsestate = groundtable;
					break;

				case CASE_RI:
					/* RI */
					fBuffer->InsertRI();
					parsestate = groundtable;
					break;

				case CASE_SS2:
					/* SS2 */
					curess = c;
					parsestate = groundtable;
					break;

				case CASE_SS3:
					/* SS3 */
					curess = c;
					parsestate = groundtable;
					break;

				case CASE_CSI_STATE:
					/* enter csi state */
					nparam = 1;
					param[0] = DEFAULT;
					parsestate = gCsiTable;
					break;

				case CASE_OSC:
					{
						/* Operating System Command: ESC ] */
						char string[512];
						uint32 len = 0;
						uchar mode_char = _NextParseChar();
						if (mode_char != '0'
								&& mode_char != '1'
								&& mode_char != '2') {
							parsestate = groundtable;
							break;
						}
						uchar currentChar = _NextParseChar();
						while ((currentChar = _NextParseChar()) != 0x7) {
							if (!isprint(currentChar & 0x7f)
									|| len+2 >= sizeof(string))
								break;
							string[len++] = currentChar;
						}
						if (currentChar == 0x7) {
							string[len] = '\0';
							switch (mode_char) {
								case '0':
								case '2':
									fBuffer->SetTitle(string);
									break;
								case '1':
									break;
							}
						}
						parsestate = groundtable;
						break;
					}

				case CASE_RIS:		// ESC c ... Reset terminal.
					break;

				case CASE_LS2:
					/* LS2 */
					//      screen->curgl = 2;
					parsestate = groundtable;
					break;

				case CASE_LS3:
					/* LS3 */
					//      screen->curgl = 3;
					parsestate = groundtable;
					break;

				case CASE_LS3R:
					/* LS3R */
					//      screen->curgr = 3;
					parsestate = groundtable;
					break;

				case CASE_LS2R:
					/* LS2R */
					//      screen->curgr = 2;
					parsestate = groundtable;
					break;

				case CASE_LS1R:
					/* LS1R */
					//      screen->curgr = 1;
					parsestate = groundtable;
					break;

				case CASE_VPA:		// ESC [...d move cursor absolute vertical
					/* VPA (CV) */
					if ((row = param[0]) < 1)
						row = 1;

					// note beterm wants it 1-based unlike usual terminals
					fBuffer->SetCursorY(row - 1);
					parsestate = groundtable;
					break;

				case CASE_HPA:		// ESC [...G move cursor absolute horizontal
					/* HPA (CH) */
					if ((column = param[0]) < 1)
						column = 1;

					// note beterm wants it 1-based unlike usual terminals
					fBuffer->SetCursorX(column - 1);
					parsestate = groundtable;
					break;

				case CASE_SU:	// scroll screen up
					if ((row = param[0]) < 1)
						row = 1;
					fBuffer->ScrollBy(row);
					parsestate = groundtable;
					break;

				case CASE_SD:	// scroll screen down
					if ((row = param[0]) < 1)
						row = 1;
					fBuffer->ScrollBy(-row);
					parsestate = groundtable;
					break;


				case CASE_ECH:	// erase characters
					if ((column = param[0]) < 1)
						column = 1;
					fBuffer->EraseChars(column);
					parsestate = groundtable;
					break;

				default:
					break;
			}
		} catch (...) {
			break;
		}
	}

	return B_OK;
}


/*static*/ int32
TermParse::_ptyreader_thread(void *data)
{
	return reinterpret_cast<TermParse *>(data)->PtyReader();
}


/*static*/ int32
TermParse::_escparse_thread(void *data)
{
	return reinterpret_cast<TermParse *>(data)->EscParse();
}


status_t
TermParse::_ReadParserBuffer()
{
	// We have to unlock the terminal buffer while waiting for data from the
	// PTY. We don't have to unlock when we don't need to wait, but we do it
	// anyway, so that TermView won't be starved when trying to synchronize.
	fBuffer->Unlock();

	// wait for new input from pty
	if (fReadBufferSize == 0) {
		status_t status = B_OK;
		while (fReadBufferSize == 0 && status == B_OK) {
			do {
				status = acquire_sem(fReaderSem);
			} while (status == B_INTERRUPTED);

			// eat any sems that were released unconditionally
			int32 semCount;
			if (get_sem_count(fReaderSem, &semCount) == B_OK && semCount > 0)
				acquire_sem_etc(fReaderSem, semCount, B_RELATIVE_TIMEOUT, 0);
		}

		if (status < B_OK) {
			fBuffer->Lock();
			return status;
		}
	}

	int32 toRead = fReadBufferSize;
	if (toRead > ESC_PARSER_BUFFER_SIZE)
		toRead = ESC_PARSER_BUFFER_SIZE;

	for (int32 i = 0; i < toRead; i++) {
		// TODO: This could be optimized using memcpy instead and
		// calculating space left as in the PtyReader().
		fParserBuffer[i] = fReadBuffer[fBufferPosition];
		fBufferPosition = (fBufferPosition + 1) % READ_BUF_SIZE;
	}

	int32 bufferSize = atomic_add(&fReadBufferSize, -toRead);

	// If the pty reader thread waits and we have made enough space in the
	// buffer now, let it run again.
	if (bufferSize > READ_BUF_SIZE - MIN_PTY_BUFFER_SPACE
			&& bufferSize - toRead <= READ_BUF_SIZE - MIN_PTY_BUFFER_SPACE) {
		release_sem(fReaderLocker);
	}

	fParserBufferSize = toRead;
	fParserBufferOffset = 0;

	fBuffer->Lock();
	return B_OK;
}


void
TermParse::_DeviceStatusReport(int n)
{
	char sbuf[16] ;
	int len;

	switch (n) {
		case 5:
			{
				// Device status report requested
				// reply with "no malfunction detected"
				const char* toWrite = "\033[0n";
				write(fFd, toWrite, strlen(toWrite));
				break ;
			}
		case 6:
			// Cursor position report requested
			len = sprintf(sbuf, "\033[%" B_PRId32 ";%" B_PRId32 "R",
					fBuffer->Cursor().y + 1,
					fBuffer->Cursor().x + 1);
			write(fFd, sbuf, len);
			break ;
		default:
			return;
	}
}


void
TermParse::_DecReqTermParms(int value)
{
	// Terminal parameters report:
	//   type (2 or 3);
	//   no parity (1);
	//   8 bits per character (1);
	//   transmit speed 38400bps (128);
	//   receive speed 38400bps (128);
	//   bit rate multiplier 16 (1);
	//   no flags (0)
	char parms[] = "\033[?;1;1;128;128;1;0x";

	if (value < 1)
		parms[2] = '2';
	else if (value == 1)
		parms[2] = '3';
	else
		return;

	write(fFd, parms, strlen(parms));
}


void
TermParse::_DecPrivateModeSet(int value)
{
	switch (value) {
		case 1:
			// Application Cursor Keys (whatever that means).
			// Not supported yet.
			break;
		case 5:
			// Reverse Video (inverses colors for the complete screen
			// -- when followed by normal video, that's shortly flashes the
			// screen).
			// Not supported yet.
			break;
		case 6:
			// Set Origin Mode.
			fBuffer->SetOriginMode(true);
			break;
		case 9:
			// Set Mouse X and Y on button press.
			fBuffer->ReportX10MouseEvent(true);
			break;
		case 12:
			// Start Blinking Cursor.
			// Not supported yet.
			break;
		case 25:
			// Show Cursor.
			// Not supported yet.
			break;
		case 47:
			// Use Alternate Screen Buffer.
			fBuffer->UseAlternateScreenBuffer(false);
			break;
		case 1000:
			// Send Mouse X & Y on button press and release.
			fBuffer->ReportNormalMouseEvent(true);
			break;
		case 1002:
			// Send Mouse X and Y on button press and release, and on motion
			// when the mouse enter a new cell
			fBuffer->ReportButtonMouseEvent(true);
			break;
		case 1003:
			// Use All Motion Mouse Tracking
			fBuffer->ReportAnyMouseEvent(true);
			break;
		case 1034:
			// TODO: Interprete "meta" key, sets eighth bit.
			// Not supported yet.
			break;
		case 1036:
			// TODO: Send ESC when Meta modifies a key
			// Not supported yet.
			break;
		case 1039:
			// TODO: Send ESC when Alt modifies a key
			// Not supported yet.
			break;
		case 1049:
			// Save cursor as in DECSC and use Alternate Screen Buffer, clearing
			// it first.
			_DecSaveCursor();
			fBuffer->UseAlternateScreenBuffer(true);
			break;
	}
}


void
TermParse::_DecPrivateModeReset(int value)
{
	switch (value) {
		case 1:
			// Normal Cursor Keys (whatever that means).
			// Not supported yet.
			break;
		case 3:
			// 80 Column Mode.
			// Not supported yet.
			break;
		case 4:
			// Jump (Fast) Scroll.
			// Not supported yet.
			break;
		case 5:
			// Normal Video (Leaves Reverse Video, cf. there).
			// Not supported yet.
			break;
		case 6:
			// Reset Origin Mode.
			fBuffer->SetOriginMode(false);
			break;
		case 9:
			// Disable Mouse X and Y on button press.
			fBuffer->ReportX10MouseEvent(false);
			break;
		case 12:
			// Stop Blinking Cursor.
			// Not supported yet.
			break;
		case 25:
			// Hide Cursor
			// Not supported yet.
			break;
		case 47:
			// Use Normal Screen Buffer.
			fBuffer->UseNormalScreenBuffer();
			break;
		case 1000:
			// Don't send Mouse X & Y on button press and release.
			fBuffer->ReportNormalMouseEvent(false);
			break;
		case 1002:
			// Don't send Mouse X and Y on button press and release, and on motion
			// when the mouse enter a new cell
			fBuffer->ReportButtonMouseEvent(false);
			break;
		case 1003:
			// Disable All Motion Mouse Tracking.
			fBuffer->ReportAnyMouseEvent(false);
			break;
		case 1034:
			// Don't interprete "meta" key.
			// Not supported yet.
			break;
		case 1036:
			// TODO: Don't send ESC when Meta modifies a key
			// Not supported yet.
			break;
		case 1039:
			// TODO: Don't send ESC when Alt modifies a key
			// Not supported yet.
			break;
		case 1049:
			// Use Normal Screen Buffer and restore cursor as in DECRC.
			fBuffer->UseNormalScreenBuffer();
			_DecRestoreCursor();
			break;
	}
}


void
TermParse::_DecSaveCursor()
{
	fBuffer->SaveCursor();
	fBuffer->SaveOriginMode();
	fSavedAttr = fAttr;
}


void
TermParse::_DecRestoreCursor()
{
	fBuffer->RestoreCursor();
	fBuffer->RestoreOriginMode();
	fAttr = fSavedAttr;
}
