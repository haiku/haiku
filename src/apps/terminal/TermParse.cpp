/*
 * Copyright 2001-2007, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 */
#include "TermParse.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <Autolock.h>
#include <Beep.h>
#include <Message.h>

#include "CodeConv.h"
#include "TermConst.h"
#include "TerminalBuffer.h"
#include "VTparse.h"


//////////////////////////////////////////////////////////////////////////////
// EscParse ... Escape sequence parse and character encoding.
//
//////////////////////////////////////////////////////////////////////////////


extern int gUTF8GroundTable[];		/* UTF8 Ground table */
extern int gCS96GroundTable[];		/* CS96 Ground table */
extern int gISO8859GroundTable[];	/* ISO8859 & EUC Ground table */
extern int gSJISGroundTable[];		/* Shift-JIS Ground table */

extern int gEscTable[];		/* ESC */
extern int gCsiTable[];		/* ESC [ */
extern int gDecTable[];		/* ESC [ ? */
extern int gScrTable[];		/* ESC # */
extern int gIgnoreTable[];		/* ignore table */
extern int gIesTable[];		/* ignore ESC table */
extern int gEscIgnoreTable[];		/* ESC ignore table */
extern int gMbcsTable[];		/* ESC $ */



#define DEFAULT -1
#define NPARAM 10		// Max parameters


//! Get char from pty reader buffer.
inline status_t
TermParse::_NextParseChar(uchar &c)
{
	if (fParserBufferOffset >= fParserBufferSize) {
		// parser buffer empty
		status_t error = _ReadParserBuffer();
		if (error != B_OK)
			return error;
	}

	c = fParserBuffer[fParserBufferOffset++];

	return B_OK;
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
	fParserWaiting(false),
	fBuffer(NULL),
	fQuitting(true)
{
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

	status_t status = InitPtyReader();
	if (status < B_OK) {
		fBuffer = NULL;	
		return status;
	}

	status = InitTermParse();
	if (status < B_OK) {
		StopPtyReader();
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

	StopPtyReader();
	StopTermParse();
	
	fBuffer = NULL;
	
	return B_OK;
}


//! Initialize and spawn EscParse thread.
status_t
TermParse::InitTermParse()
{
	if (fParseThread >= 0)
		return B_ERROR; // we might want to return B_OK instead ?

	fParseThread = spawn_thread(_escparse_thread, "EscParse",
		B_DISPLAY_PRIORITY, this);

	resume_thread(fParseThread);
	
	return B_OK;
}


//! Initialize and spawn PtyReader thread.
status_t
TermParse::InitPtyReader()
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
TermParse::StopTermParse()
{
	if (fParseThread >= 0) {
		status_t dummy;
		wait_for_thread(fParseThread, &dummy);
		fParseThread = -1;	
	}
}


void
TermParse::StopPtyReader()
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
		if (bufferSize == 0 && fParserWaiting)
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
	for (i = 0; tables[i].p; i++)
		if (tables[i].p == groundtable)
			fprintf(stderr, "%s\t", tables[i].name);
	fprintf(stderr, "parsestate: ");
	for (i = 0; tables[i].p; i++)
		if (tables[i].p == parsestate)
			fprintf(stderr, "%s\t", tables[i].name);
	fprintf(stderr, "char: 0x%02x (%d)\n", c, c);
}


int32
TermParse::EscParse()
{
	int tmp;
	int top, bot;
	int cs96 = 0;
	uchar curess = 0;
	
	char cbuf[4], dstbuf[4];
	char *ptr;

	int now_coding = -1;

	ushort attr = BACKCOLOR;

	int param[NPARAM];
	int nparam = 1;

	int row, col;

	/* default coding system is UTF8 */
	int *groundtable = gUTF8GroundTable;
	int *parsestate = groundtable;

	BAutolock locker(fBuffer);

	while (!fQuitting) {
		uchar c;
		if (_NextParseChar(c) < B_OK)
			break;

		//DumpState(groundtable, parsestate, c);

		if (now_coding != fBuffer->Encoding()) {
			/*
			 * Change coding, change parse table.
			 */
			switch (fBuffer->Encoding()) {
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
					groundtable = gISO8859GroundTable;
					break;
				case B_SJIS_CONVERSION:
					groundtable = gSJISGroundTable;
					break;
				case B_EUC_CONVERSION:
				case B_EUC_KR_CONVERSION:
				case B_JIS_CONVERSION:
					groundtable = gISO8859GroundTable;
					break;
				case M_UTF8:
				default:
					groundtable = gUTF8GroundTable;
					break;
			}
			parsestate = groundtable;
			now_coding = fBuffer->Encoding();
    	}

//debug_printf("TermParse: char: '%c' (%d), parse state: %d\n", c, c, parsestate[c]);
		switch (parsestate[c]) {
			case CASE_PRINT:
				fBuffer->InsertChar((char)c, attr);
				break;

			case CASE_PRINT_GR:
				/* case iso8859 gr character, or euc */
				ptr = cbuf;
				if (now_coding == B_EUC_CONVERSION || now_coding == B_EUC_KR_CONVERSION
					|| now_coding == B_JIS_CONVERSION) {
					switch (parsestate[curess]) {
						case CASE_SS2:		/* JIS X 0201 */
							*ptr++ = curess;
							*ptr++ = c;
							*ptr = 0;
							curess = 0;
							break;

						case CASE_SS3:		/* JIS X 0212 */
							*ptr++ = curess;
							*ptr++ = c;
							_NextParseChar(c);
							*ptr++ = c;
							*ptr = 0;
							curess = 0;
							break;

						default:		/* JIS X 0208 */
							*ptr++ = c;
							_NextParseChar(c);
							*ptr++ = c;
							*ptr = 0;
							break;
					}
				} else {
					/* ISO-8859-1...10 and MacRoman */
					*ptr++ = c;
					*ptr = 0;
				}

				if (now_coding != B_JIS_CONVERSION) {
					CodeConv::ConvertToInternal(cbuf, -1, dstbuf, now_coding);
				} else {
					CodeConv::ConvertToInternal(cbuf, -1, dstbuf,
						B_EUC_CONVERSION);
				}

				fBuffer->InsertChar(dstbuf, 4, attr);
				break;

			case CASE_PRINT_CS96:
				cbuf[0] = c | 0x80;
				_NextParseChar(c);
				cbuf[1] = c | 0x80;
				cbuf[2] = 0;
				CodeConv::ConvertToInternal(cbuf, 2, dstbuf, B_EUC_CONVERSION);
				fBuffer->InsertChar(dstbuf, 4, attr);
				break;

			case CASE_LF:
				fBuffer->InsertLF();
				break;

			case CASE_CR:
				fBuffer->InsertCR();
				break;

			case CASE_SJIS_KANA:
				cbuf[0] = c;
				cbuf[1] = '\0';
				CodeConv::ConvertToInternal(cbuf, 1, dstbuf, now_coding);
				fBuffer->InsertChar(dstbuf, 4, attr);
				break;

			case CASE_SJIS_INSTRING:
				cbuf[0] = c;
				_NextParseChar(c);
				cbuf[1] = c;
				cbuf[2] = '\0';
				CodeConv::ConvertToInternal(cbuf, 2, dstbuf, now_coding);
				fBuffer->InsertChar(dstbuf, 4, attr);
				break;

			case CASE_UTF8_2BYTE:
				cbuf[0] = c;
				_NextParseChar(c);
				if (groundtable[c] != CASE_UTF8_INSTRING)
					break;
				cbuf[1] = c;
				cbuf[2] = '\0';
				
				fBuffer->InsertChar(cbuf, 2, attr);
				break;

			case CASE_UTF8_3BYTE:
				cbuf[0] = c;
				_NextParseChar(c);
				if (groundtable[c] != CASE_UTF8_INSTRING)
					break;
				cbuf[1] = c;

				_NextParseChar(c);
				if (groundtable[c] != CASE_UTF8_INSTRING)
					break;
				cbuf[2] = c;
				cbuf[3] = '\0';
				fBuffer->InsertChar(cbuf, 3, attr);
				break;

			case CASE_MBCS:
				/* ESC $ */
				parsestate = gMbcsTable;
				break;

			case CASE_GSETS:
				/* ESC $ ? */
				parsestate = gCS96GroundTable;
				cs96 = 1;
				break;

			case CASE_SCS_STATE:
			{
				cs96 = 0;
				uchar dummy;				
				_NextParseChar(dummy);
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
				tmp = fBuffer->Cursor().x;
				tmp %= 8;
				fBuffer->MoveCursorRight(8 - tmp);
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
				break;

			case CASE_SO:
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
				if (nparam < 2 || (col = param[1]) < 1)
					col = 1;

				fBuffer->SetCursor(col - 1, row - 1 );
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
						fBuffer->EraseBelow();
						fBuffer->EraseAbove();
						break;
				}
				parsestate = groundtable;
				break;

			case CASE_EL:		// delete line
				/* EL */
				fBuffer->DeleteColumns();
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
				fBuffer->SetInsertMode(MODE_INSERT);
				parsestate = groundtable;
				break;

			case CASE_RST:
				/* RST */
				fBuffer->SetInsertMode(MODE_OVER);
				parsestate = groundtable;
				break;

			case CASE_SGR:
				/* SGR */
				for (row = 0; row < nparam; ++row) {
					switch (param[row]) {
						case DEFAULT:
						case 0: /* Reset attribute */
							attr = 0;
							break;

						case 1:
						case 5:	/* Bold		*/
							attr |= BOLD;
							break;

						case 4:	/* Underline	*/
							attr |= UNDERLINE;
							break;

						case 7:	/* Inverse	*/
							attr |= INVERSE;
							break;

						case 22:	/* Not Bold	*/
							attr &= ~BOLD;
							break;

						case 24:	/* Not Underline	*/
							attr &= ~UNDERLINE;
							break;

						case 27:	/* Not Inverse	*/
							attr &= ~INVERSE;
							break;

						case 30:
						case 31:
						case 32:
						case 33:
						case 34:
						case 35:
						case 36:
						case 37:
							attr &= ~FORECOLOR;
							attr |= FORECOLORED(param[row] - 30);
							attr |= FORESET;
							break;

						case 39:
							attr &= ~FORESET;
							break;

						case 40:
						case 41:
						case 42:
						case 43:
						case 44:
						case 45:
						case 46:
						case 47:
							attr &= ~BACKCOLOR;
							attr |= BACKCOLORED(param[row] - 40);
							attr |= BACKSET;
							break;

						case 49:
							attr &= ~BACKSET;
							break;
					}
				}
				parsestate = groundtable;
				break;

				case CASE_CPR:
					// Q & D hack by Y.Hayakawa (hida@sawada.riec.tohoku.ac.jp)
					// 21-JUL-99
					_DeviceStatusReport(param[0]);
					parsestate = groundtable;
					break;

				case CASE_DECSTBM:
					/* DECSTBM - set scrolling region */

					if ((top = param[0]) < 1)
						top = 1;

					if (nparam < 2)
						bot = fBuffer->Height();
					else
						bot = param[1];

					top--;
					bot--;

					if (bot > top)
						fBuffer->SetScrollRegion(top, bot);

					parsestate = groundtable;
					break;

				case CASE_DECREQTPARM:
					parsestate = groundtable;
					break;

				case CASE_DECSET:
					/* DECSET */
					//      dpmodes(term, bitset);
					parsestate = groundtable;
					break;

				case CASE_DECRST:
					/* DECRST */
					//      dpmodes(term, bitclr);
					parsestate = groundtable;
					break;

				case CASE_DECALN:
					/* DECALN */
					//      if(screen->cursor_state)
					//	HideCursor();
					//      ScrnRefresh(screen, 0, 0, screen->max_row + 1,
					//		  screen->max_col + 1, False);
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
					//      TabSet(term->tabs, screen->cur_col);
					parsestate = groundtable;
					break;

				case CASE_RI:
					/* RI */
					fBuffer->ScrollBy(-1);
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
					uchar mode_char;
					_NextParseChar(mode_char);
					if (mode_char != '0'
						&& mode_char != '1' 
						&& mode_char != '2') {
						parsestate = groundtable;
						break;
					}
					uchar current_char;
					_NextParseChar(current_char);
					while (_NextParseChar(current_char) == B_OK 
						&& current_char != 0x7) {
						if (!isprint(current_char & 0x7f)
							|| len+2 >= sizeof(string))
							break;
						string[len++] = current_char;
					}
					if (current_char == 0x7) {
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
					if ((col = param[0]) < 1)
						col = 1;

					// note beterm wants it 1-based unlike usual terminals
					fBuffer->SetCursorX(col - 1);
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

				default:
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
		fParserWaiting = true;

		status_t status = B_OK;
		while (fReadBufferSize == 0 && status == B_OK) {
			do {
				status = acquire_sem(fReaderSem);
			} while (status == B_INTERRUPTED);
		}

		fParserWaiting = false;

		if (status < B_OK) {
			fBuffer->Lock();
			return status;
		}
	}

	int32 toRead = fReadBufferSize;
	if (toRead > ESC_PARSER_BUFFER_SIZE)
		toRead = ESC_PARSER_BUFFER_SIZE;

	for (int32 i = 0; i < toRead; i++) {
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
			const char* toWrite = "\033[0n";
			write(fFd, toWrite, strlen(toWrite));
			break ;
		}
		case 6:
			len = sprintf(sbuf, "\033[%ld;%ldR", fBuffer->Height(),
				fBuffer->Width()) ;
			write(fFd, sbuf, len);
			break ;
		default:
			return;
	}
}
