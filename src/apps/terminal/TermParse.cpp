/*
 * Copyright 2001-2013, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Kian Duffy, myob@users.sourceforge.net
 *		Simon South, simon@simonsouth.net
 *		Siarzhuk Zharski, zharik@gmx.li
 */


//! Escape sequence parse and character encoding.


#include "TermParse.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <Autolock.h>
#include <Beep.h>
#include <Catalog.h>
#include <Locale.h>
#include <Message.h>
#include <UTF8.h>

#include "Colors.h"
#include "TermConst.h"
#include "TerminalBuffer.h"
#include "VTparse.h"


extern int gUTF8GroundTable[];		/* UTF8 Ground table */
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

extern const char* gLineDrawGraphSet[]; /* may be used for G0, G1, G2, G3 */

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

#ifdef USE_DEBUG_SNAPSHOTS
	fBuffer->CaptureChar(fParserBuffer[fParserBufferOffset]);
#endif

	return fParserBuffer[fParserBufferOffset++];
}


TermParse::TermParse(int fd)
	:
	fFd(fd),
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
		case B_BIG5_CONVERSION:
			return gISO8859GroundTable;

		case B_KOI8R_CONVERSION:
		case B_MS_WINDOWS_1251_CONVERSION:
		case B_MS_WINDOWS_CONVERSION:
		case B_MAC_ROMAN_CONVERSION:
		case B_MS_DOS_866_CONVERSION:
		case B_GBK_CONVERSION:
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
	int top = 0;
	int bottom = 0;

	char cbuf[4] = { 0 };
	char dstbuf[4] = { 0 };

	int currentEncoding = -1;

	int param[NPARAM];
	int nparam = 1;
	for (int i = 0; i < NPARAM; i++)
		param[i] = DEFAULT;

	int row = 0;
	int column = 0;

	// default encoding system is UTF8
	int *groundtable = gUTF8GroundTable;
	int *parsestate = gUTF8GroundTable;

	// handle alternative character sets G0 - G4
	const char** graphSets[4] = { NULL, NULL, NULL, NULL };
	int curGL = 0;
	int curGR = 0;

	BAutolock locker(fBuffer);

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
			int32 srcLen = 0;
			int32 dstLen = sizeof(dstbuf);
			int32 dummyState = 0;

			switch (parsestate[c]) {
				case CASE_PRINT:
				{
					int curGS = c < 128 ? curGL : curGR;
					const char** curGraphSet = graphSets[curGS];
					if (curGraphSet != NULL) {
						int offset = c - (c < 128 ? 0x20 : 0xA0);
						if (offset >= 0 && offset < 96
							&& curGraphSet[offset] != 0) {
							fBuffer->InsertChar(curGraphSet[offset]);
							break;
						}
					}
					fBuffer->InsertChar((char)c);
					break;
				}
				case CASE_PRINT_GR:
				{
					/* case iso8859 gr character, or euc */
					switch (currentEncoding) {
						case B_EUC_CONVERSION:
						case B_EUC_KR_CONVERSION:
						case B_JIS_CONVERSION:
						case B_BIG5_CONVERSION:
							cbuf[srcLen++] = c;
							c = _NextParseChar();
							cbuf[srcLen++] = c;
							break;

						case B_GBK_CONVERSION:
							cbuf[srcLen++] = c;
							do {
								// GBK-compatible codepoints are 2-bytes long
								c = _NextParseChar();
								cbuf[srcLen++] = c;

								// GB18030 extends GBK with 4-byte codepoints
								// using 2nd byte from range 0x30...0x39
								if (srcLen == 2 && (c < 0x30 || c > 0x39))
									break;
							} while (srcLen < 4);
							break;

						default: // ISO-8859-1...10 and MacRoman
							cbuf[srcLen++] = c;
							break;
					}

					if (srcLen > 0) {
						int encoding = currentEncoding == B_JIS_CONVERSION
							? B_EUC_CONVERSION : currentEncoding;

						convert_to_utf8(encoding, cbuf, &srcLen,
								dstbuf, &dstLen, &dummyState, '?');

						fBuffer->InsertChar(UTF8Char(dstbuf, dstLen));
					}
					break;
				}

				case CASE_LF:
					fBuffer->InsertLF();
					break;

				case CASE_CR:
					fBuffer->InsertCR();
					break;

				case CASE_INDEX:
					fBuffer->InsertLF();
					parsestate = groundtable;
					break;

				case CASE_NEXT_LINE:
					fBuffer->NextLine();
					parsestate = groundtable;
					break;

				case CASE_SJIS_KANA:
					cbuf[srcLen++] = c;
					convert_to_utf8(currentEncoding, cbuf, &srcLen,
							dstbuf, &dstLen, &dummyState, '?');
					fBuffer->InsertChar(UTF8Char(dstbuf, dstLen));
					break;

				case CASE_SJIS_INSTRING:
					cbuf[srcLen++] = c;
					c = _NextParseChar();
					cbuf[srcLen++] = c;

					convert_to_utf8(currentEncoding, cbuf, &srcLen,
							dstbuf, &dstLen, &dummyState, '?');
					fBuffer->InsertChar(UTF8Char(dstbuf, dstLen));
					break;

				case CASE_UTF8_2BYTE:
					cbuf[srcLen++] = c;
					c = _NextParseChar();
					if (groundtable[c] != CASE_UTF8_INSTRING)
						break;
					cbuf[srcLen++] = c;

					fBuffer->InsertChar(UTF8Char(cbuf, srcLen));
					break;

				case CASE_UTF8_3BYTE:
					cbuf[srcLen++] = c;

					do {
						c = _NextParseChar();
						if (groundtable[c] != CASE_UTF8_INSTRING) {
							srcLen = 0;
							break;
						}
						cbuf[srcLen++] = c;

					} while (srcLen != 3);

					if (srcLen > 0)
						fBuffer->InsertChar(UTF8Char(cbuf, srcLen));
					break;

				case CASE_SCS_STATE:
				{
					int set = -1;
					switch (c) {
						case '(':
							set = 0;
							break;
						case ')':
						case '-':
							set = 1;
							break;
						case '*':
						case '.':
							set = 2;
							break;
						case '+':
						case '/':
							set = 3;
							break;
						default:
							break;
					}

					if (set > -1) {
						char page = _NextParseChar();
						switch (page) {
							case '0':
								graphSets[set] = gLineDrawGraphSet;
								break;
							default:
								graphSets[set] = NULL;
								break;
						}
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
					fBuffer->InsertTab();
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

				case CASE_LS1:
					/* select G1 into GL */
					curGL = 1;
					parsestate = groundtable;
					break;

				case CASE_LS0:
					/* select G0 into GL */
					curGL = 0;
					parsestate = groundtable;
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

				case CASE_CSI_SP: // ESC [N q
					// part of change cursor style DECSCUSR
					if (nparam < NPARAM)
						param[nparam++] = ' ';
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
					uint32 attributes = fBuffer->GetAttributes();
					for (row = 0; row < nparam; ++row) {
						switch (param[row]) {
							case DEFAULT:
							case 0: /* Reset attribute */
								attributes = 0;
								break;

							case 1: /* Bold     */
							case 5:
								attributes |= BOLD;
								break;

							case 4:	/* Underline	*/
								attributes |= UNDERLINE;
								break;

							case 7:	/* Inverse	*/
								attributes |= INVERSE;
								break;

							case 22:	/* Not Bold	*/
								attributes &= ~BOLD;
								break;

							case 24:	/* Not Underline	*/
								attributes &= ~UNDERLINE;
								break;

							case 27:	/* Not Inverse	*/
								attributes &= ~INVERSE;
								break;

							case 90:
							case 91:
							case 92:
							case 93:
							case 94:
							case 95:
							case 96:
							case 97:
								param[row] -= 60;
							case 30:
							case 31:
							case 32:
							case 33:
							case 34:
							case 35:
							case 36:
							case 37:
								attributes &= ~FORECOLOR;
								attributes |= FORECOLORED(param[row] - 30);
								attributes |= FORESET;
								break;

							case 38:
							{
								int color = -1;
								if (nparam == 3 && param[1] == 5)
									color = param[2];
								else if (nparam == 5 && param[1] == 2)
									color = fBuffer->GuessPaletteColor(
										param[2], param[3], param[4]);

								if (color >= 0) {
									attributes &= ~FORECOLOR;
									attributes |= FORECOLORED(color);
									attributes |= FORESET;
								}

								row = nparam; // force exit of the parsing
								break;
							}

							case 39:
								attributes &= ~FORESET;
								break;

							case 100:
							case 101:
							case 102:
							case 103:
							case 104:
							case 105:
							case 106:
							case 107:
								param[row] -= 60;
							case 40:
							case 41:
							case 42:
							case 43:
							case 44:
							case 45:
							case 46:
							case 47:
								attributes &= ~BACKCOLOR;
								attributes |= BACKCOLORED(param[row] - 40);
								attributes |= BACKSET;
								break;

							case 48:
							{
								int color = -1;
								if (nparam == 3 && param[1] == 5)
									color = param[2];
								else if (nparam == 5 && param[1] == 2)
									color = fBuffer->GuessPaletteColor(
										param[2], param[3], param[4]);

								if (color >= 0) {
									attributes &= ~BACKCOLOR;
									attributes |= BACKCOLORED(color);
									attributes |= BACKSET;
								}

								row = nparam; // force exit of the parsing
								break;
							}

							case 49:
								attributes &= ~BACKSET;
								break;
						}
					}
					fBuffer->SetAttributes(attributes);
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

				case CASE_DECSCUSR_ETC:
				// DECSCUSR - set cursor style VT520
					if (nparam == 2 && param[1] == ' ') {
						bool blinking = (param[0] & 0x01) != 0;
						int style = -1;
						switch (param[0]) {
							case 0:
								blinking = true;
							case 1:
							case 2:
								style = BLOCK_CURSOR;
								break;
							case 3:
							case 4:
								style = UNDERLINE_CURSOR;
								break;
							case 5:
							case 6:
								style = IBEAM_CURSOR;
								break;
						}

						if (style != -1)
							fBuffer->SetCursorStyle(style, blinking);
					}
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
					fBuffer->FillScreen(UTF8Char('E'), 0);
					parsestate = groundtable;
					break;

					//	case CASE_GSETS:
					//		screen->gsets[scstype] = GSET(c) | cs96;
					//		parsestate = groundtable;
					//		break;

				case CASE_DECSC:
					/* DECSC */
					fBuffer->SaveCursor();
					parsestate = groundtable;
					break;

				case CASE_DECRC:
					/* DECRC */
					fBuffer->RestoreCursor();
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
					parsestate = groundtable;
					break;

				case CASE_SS3:
					/* SS3 */
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
						uchar params[512];
						// fill the buffer until BEL, ST or something else.
						bool isParsed = false;
						int32 skipCount = 0; // take care about UTF-8 characters
						for (uint i = 0; !isParsed && i < sizeof(params); i++) {
							params[i] = _NextParseChar();

							if (skipCount > 0) {
								skipCount--;
								continue;
							}

							skipCount = UTF8Char::ByteCount(params[i]) - 1;
							if (skipCount > 0)
								continue;

							switch (params[i]) {
								// BEL
								case 0x07:
									isParsed = true;
									break;
								// 8-bit ST
								case 0x9c:
									isParsed = true;
									break;
								// 7-bit ST is "ESC \"
								case '\\':
								// hm... Was \x1b replaced by 0 during parsing?
									if (i > 0 && params[i - 1] == 0) {
										isParsed = true;
										break;
									}
								default:
									if (!isprint(params[i] & 0x7f))
										break;
									continue;
							}
							params[i] = '\0';
						}

						// watchdog for the 'end of buffer' case
						params[sizeof(params) - 1] = '\0';

						if (isParsed)
							_ProcessOperatingSystemControls(params);

						parsestate = groundtable;
						break;
					}

				case CASE_RIS:		// ESC c ... Reset terminal.
					break;

				case CASE_LS2:
					/* select G2 into GL */
					curGL = 2;
					parsestate = groundtable;
					break;

				case CASE_LS3:
					/* select G3 into GL */
					curGL = 3;
					parsestate = groundtable;
					break;

				case CASE_LS3R:
					/* select G3 into GR */
					curGR = 3;
					parsestate = groundtable;
					break;

				case CASE_LS2R:
					/* select G2 into GR */
					curGR = 2;
					parsestate = groundtable;
					break;

				case CASE_LS1R:
					/* select G1 into GR */
					curGR = 1;
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

				case CASE_CBT:	// cursor back tab
					if ((column = param[0]) < 1)
						column = 1;
					fBuffer->InsertCursorBackTab(column);
					parsestate = groundtable;
					break;

				case CASE_CFT:	// cursor forward tab
					if ((column= param[0]) < 1)
						column = 1;
					for (int32 i = 0; i < column; ++i)
						fBuffer->InsertTab();
					parsestate = groundtable;
					break;

				case CASE_CNL:	// cursor next line
					if ((row= param[0]) < 1)
						row = 1;
					fBuffer->SetCursorX(0);
					fBuffer->MoveCursorDown(row);
					parsestate = groundtable;
					break;

				case CASE_CPL:	// cursor previous line
					if ((row= param[0]) < 1)
						row = 1;
					fBuffer->SetCursorX(0);
					fBuffer->MoveCursorUp(row);
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
	if (atomic_get(&fReadBufferSize) == 0) {
		status_t status = B_OK;
		while (atomic_get(&fReadBufferSize) == 0 && status == B_OK) {
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

	int32 toRead = atomic_get(&fReadBufferSize);
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
			len = snprintf(sbuf, sizeof(sbuf),
				"\033[%" B_PRId32 ";%" B_PRId32 "R",
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
			fBuffer->SetCursorBlinking(true);
			break;
		case 25:
			// Show Cursor.
			fBuffer->SetCursorHidden(false);
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
		case 1006:
			// Enable extended mouse coordinates with SGR scheme
			fBuffer->EnableExtendedMouseCoordinates(true);
			break;
		case 1034:
			// Interpret "meta" key, sets eighth bit.
			fBuffer->EnableInterpretMetaKey(true);
			break;
		case 1036:
			// Send ESC when Meta modifies a key
			fBuffer->EnableMetaKeySendsEscape(true);
			break;
		case 1039:
			// TODO: Send ESC when Alt modifies a key
			// Not supported yet.
			break;
		case 1049:
			// Save cursor as in DECSC and use Alternate Screen Buffer, clearing
			// it first.
			fBuffer->SaveCursor();
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
			fBuffer->SetCursorBlinking(false);
			break;
		case 25:
			// Hide Cursor
			fBuffer->SetCursorHidden(true);
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
		case 1006:
			// Disable extended mouse coordinates with SGR scheme
			fBuffer->EnableExtendedMouseCoordinates(false);
			break;
		case 1034:
			// Don't interpret "meta" key.
			fBuffer->EnableInterpretMetaKey(false);
			break;
		case 1036:
			// Don't send ESC when Meta modifies a key
			fBuffer->EnableMetaKeySendsEscape(false);
			break;
		case 1039:
			// TODO: Don't send ESC when Alt modifies a key
			// Not supported yet.
			break;
		case 1049:
			// Use Normal Screen Buffer and restore cursor as in DECRC.
			fBuffer->UseNormalScreenBuffer();
			fBuffer->RestoreCursor();
			break;
	}
}


void
TermParse::_ProcessOperatingSystemControls(uchar* params)
{
	int mode = 0;
	for (uchar c = *params; c != ';' && c != '\0'; c = *(++params)) {
		mode *= 10;
		mode += c - '0';
	}

	// eat the separator
	if (*params == ';')
		params++;

	static uint8 indexes[kTermColorCount];
	static rgb_color colors[kTermColorCount];

	switch (mode) {
		case 0: // icon name and window title
		case 2: // window title
			fBuffer->SetTitle((const char*)params);
			break;
		case 4: // set colors (0 - 255)
		case 104: // reset colors (0 - 255)
			{
				bool reset = (mode / 100) == 1;

				// colors can be in "idx1:name1;...;idxN:nameN;" sequence too!
				uint32 count = 0;
				char* p = strtok((char*)params, ";");
				while (p != NULL && count < kTermColorCount) {
					indexes[count] = atoi(p);

					if (!reset) {
						p = strtok(NULL, ";");
						if (p == NULL)
							break;

						if (gXColorsTable.LookUpColor(p, &colors[count]) == B_OK)
							count++;
					} else
						count++;

					p = strtok(NULL, ";");
				};

				if (count > 0) {
					if (!reset)
						fBuffer->SetColors(indexes, colors, count);
					else
						fBuffer->ResetColors(indexes, count);
				}
			}
			break;
		// set dynamic colors (10 - 19)
		case 10: // text foreground
		case 11: // text background
			{
				int32 offset = mode - 10;
				int32 count = 0;
				char* p = strtok((char*)params, ";");
				do {
					if (gXColorsTable.LookUpColor(p, &colors[count]) != B_OK) {
						// dyna-colors are pos-sensitive - no chance to continue
						break;
					}

					indexes[count] = 10 + offset + count;
					count++;
					p = strtok(NULL, ";");

				} while (p != NULL && (offset + count) < 10);

				if (count > 0) {
					fBuffer->SetColors(indexes, colors, count, true);
				}
			}
			break;
		// reset dynamic colors (10 - 19)
		case 110: // text foreground
		case 111: // text background
			{
				indexes[0] = mode;
				fBuffer->ResetColors(indexes, 1, true);
			}
			break;
		default:
		//	printf("%d -> %s\n", mode, params);
			break;
	}
}
