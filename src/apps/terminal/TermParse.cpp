/*
 * Copyright 2001-2007, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 */
#include "TermParse.h"

#include "CodeConv.h"
#include "TermConst.h"
#include "TermView.h"
#include "VTparse.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <Beep.h>
#include <Message.h>


//////////////////////////////////////////////////////////////////////////////
// EscParse ... Escape sequence parse and character encoding.
//
//////////////////////////////////////////////////////////////////////////////


extern int utf8_groundtable[];		/* UTF8 Ground table */
extern int cs96_groundtable[];		/* CS96 Ground table */
extern int iso8859_groundtable[];	/* ISO8859 & EUC Ground table */
extern int sjis_groundtable[];		/* Shift-JIS Ground table */

extern int esctable[];		/* ESC */
extern int csitable[];		/* ESC [ */
extern int dectable[];		/* ESC [ ? */
extern int scrtable[];		/* ESC # */
extern int igntable[];		/* ignore table */
extern int iestable[];		/* ignore ESC table */
extern int eigtable[];		/* ESC ignore table */
extern int mbcstable[];		/* ESC $ */



#define DEFAULT -1
#define NPARAM 10		// Max parameters


TermParse::TermParse(int fd)
	:
	fFd(fd),
	fParseThread(-1),
	fReaderThread(-1),
	fReaderSem(-1),
	fReaderLocker(-1),
	fBufferPosition(0),	
	fLockFlag(0),
	fView(NULL),
	fQuitting(true)
{
}


TermParse::~TermParse()
{
	StopThreads();
}


status_t
TermParse::StartThreads(TermView *view)
{
	if (fView != NULL)
		return B_ERROR;

	fQuitting = false;
	fView = view;

	status_t status = InitPtyReader();
	if (status < B_OK) {
		fView = NULL;	
		return status;
	}

	status = InitTermParse();
	if (status < B_OK) {
		StopPtyReader();
		fView = NULL;
		return status;		
	}
	
	return B_OK;
}


status_t
TermParse::StopThreads()
{
	if (fView == NULL)
		return B_ERROR;

	fQuitting = true;

	StopPtyReader();
	StopTermParse();
	
	fView = NULL;
	
	return B_OK;
}


//! Get char from pty reader buffer.
status_t
TermParse::GetReaderBuf(uchar &c)
{
	status_t status;
	do {
		status = acquire_sem_etc(fReaderSem, 1, B_TIMEOUT, 10000);
	} while (status == B_INTERRUPTED);

	if (status == B_TIMED_OUT) {
		fView->ScrollAtCursor();
		fView->UpdateLine();

		// Reset cursor blinking time and turn on cursor blinking.
		fView->SetCurDraw(true);

		// wait new input from pty.
		do {
			status = acquire_sem(fReaderSem);
		} while (status == B_INTERRUPTED);
		if (status < B_OK)
			return status;
	} else if (status == B_OK) {		
		// Do nothing
	} else
		return status;

	c = fReadBuffer[fBufferPosition % READ_BUF_SIZE];
	fBufferPosition++;
  	// If PtyReader thread locked, decrement counter and unlock thread.
	if (fLockFlag != 0) {
		if (--fLockFlag == 0)
			release_sem(fReaderLocker);
	}

	fView->SetCurDraw(false);
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
	uint read_p = 0;
	while (!fQuitting) {
		// If Pty Buffer nearly full, snooze this thread, and continue.
		if ((read_p - fBufferPosition) > READ_BUF_SIZE - 16) {
			fLockFlag = READ_BUF_SIZE / 2;
			status_t status;
			do {			
				status = acquire_sem(fReaderLocker);
			} while (status == B_INTERRUPTED);
			if (status < B_OK)
				return status;		
		}

		// Read PTY
		uchar buf[READ_BUF_SIZE];
		int nread = read(fFd, buf, READ_BUF_SIZE - (read_p - fBufferPosition));
		if (nread <= 0) {
			fView->NotifyQuit(errno);
			return B_OK;
		}

		// Copy read string to PtyBuffer.

		int left = READ_BUF_SIZE - (read_p % READ_BUF_SIZE);
		int mod = read_p % READ_BUF_SIZE;

		if (nread >= left) {
			memcpy(fReadBuffer + mod, buf, left);
			memcpy(fReadBuffer, buf + left, nread - left);
		} else
			memcpy(fReadBuffer + mod, buf, nread);

		read_p += nread;

		// Release semaphore. Number of semaphore counter is nread.
		release_sem_etc(fReaderSem, nread, 0);
	}

	return B_OK;
}


int32
TermParse::EscParse()
{
	int tmp;
	int top, bot;
	int cs96;
	uchar curess = 0;
	
	uchar cbuf[4], dstbuf[4];
	uchar *ptr;

	int now_coding = -1;

	ushort attr = BACKCOLOR;

	int param[NPARAM];
	int nparam = 1;

	int row, col;

	/* default coding system is UTF8 */
	int *groundtable = utf8_groundtable;
	int *parsestate = groundtable;

	while (!fQuitting) {
		uchar c;
		if (GetReaderBuf(c) < B_OK)
			break;

		if (now_coding != fView->Encoding()) {
			/*
			 * Change coding, change parse table.
			 */
			switch (fView->Encoding()) {
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
					groundtable = iso8859_groundtable;
					break;
				case B_SJIS_CONVERSION:
					groundtable = sjis_groundtable;
					break;
				case B_EUC_CONVERSION:
				case B_EUC_KR_CONVERSION:
				case B_JIS_CONVERSION:
					groundtable = iso8859_groundtable;
					break;
				case M_UTF8:
				default:
					groundtable = utf8_groundtable;
					break;
			}
			parsestate = groundtable;
			now_coding = fView->Encoding();
    	}

		switch (parsestate[c]) {
			case CASE_PRINT:
				cbuf[0] = c;
				cbuf[1] = '\0';
				fView->Insert(cbuf, attr);
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
							GetReaderBuf(*ptr++);
							*ptr = 0;
							curess = 0;
							break;

						default:		/* JIS X 0208 */
							*ptr++ = c;
							GetReaderBuf(*ptr++);
							*ptr = 0;
							break;
					}
				} else {
					/* ISO-8859-1...10 and MacRoman */
					*ptr++ = c;
					*ptr = 0;
				}

				if (now_coding != B_JIS_CONVERSION)
					CodeConv::ConvertToInternal((char*)cbuf, -1, (char*)dstbuf, now_coding);
				else
					CodeConv::ConvertToInternal((char*)cbuf, -1, (char*)dstbuf, B_EUC_CONVERSION);

				fView->Insert(dstbuf, attr);
				break;

			case CASE_PRINT_CS96:
				cbuf[0] = c | 0x80;
				GetReaderBuf(cbuf[1]);
				cbuf[1] |= 0x80;
				cbuf[2] = 0;
				CodeConv::ConvertToInternal((char*)cbuf, 2, (char*)dstbuf, B_EUC_CONVERSION);
				fView->Insert(dstbuf, attr);
				break;

			case CASE_LF:
				fView->InsertLF();
				break;

			case CASE_CR:
				fView->InsertCR();
				break;

			case CASE_SJIS_KANA:
				cbuf[0] = (uchar)c;
				cbuf[1] = '\0';
				CodeConv::ConvertToInternal((char*)cbuf, 1, (char*)dstbuf, now_coding);
				fView->Insert(dstbuf, attr);
				break;

			case CASE_SJIS_INSTRING:
				cbuf[0] = (uchar)c;
				GetReaderBuf(cbuf[1]);
				cbuf[2] = '\0';
				CodeConv::ConvertToInternal((char*)cbuf, 2, (char*)dstbuf, now_coding);
				fView->Insert(dstbuf, attr);
				break;

			case CASE_UTF8_2BYTE:
				cbuf[0] = (uchar)c;
				GetReaderBuf(c);
				if (groundtable[c] != CASE_UTF8_INSTRING)
					break;
				cbuf[1] = (uchar)c;
				cbuf[2] = '\0';
				
				fView->Insert(cbuf, attr);
				break;

			case CASE_UTF8_3BYTE:
				cbuf[0] = c;
				GetReaderBuf(c);
				if (groundtable[c] != CASE_UTF8_INSTRING)
					break;
				cbuf[1] = c;

				GetReaderBuf(c);
				if (groundtable[c] != CASE_UTF8_INSTRING)
					break;
				cbuf[2] = c;
				cbuf[3] = '\0';
				fView->Insert(cbuf, attr);
				break;

			case CASE_MBCS:
				/* ESC $ */
				parsestate = mbcstable;
				break;

			case CASE_GSETS:
				/* ESC $ ? */
				parsestate = cs96_groundtable;
				cs96 = 1;
				break;

			case CASE_SCS_STATE:
			{
				cs96 = 0;
				uchar dummy;				
				GetReaderBuf(dummy);
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
				fView->MoveCurLeft(1);
				break;

			case CASE_TAB:
				tmp = fView->GetCurX();
				tmp %= 8;
				fView->MoveCurRight(8 - tmp);
				break;

			case CASE_ESC:
				/* escape */
				parsestate = esctable;
				break;

			case CASE_IGNORE_STATE:
				/* Ies: ignore anything else */
				parsestate = igntable;
				break;

			case CASE_IGNORE_ESC:
				/* Ign: escape */
				parsestate = iestable;
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
				parsestate = scrtable;
				break;

			case CASE_ESC_IGNORE:
				/* unknown escape sequence */
				parsestate = eigtable;
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
				parsestate = dectable;
				break;

			case CASE_ICH:		// ESC [@ insert charactor
				/* ICH */
				if ((row = param[0]) < 1)
					row = 1;
				fView->InsertSpace(row);
				parsestate = groundtable;
				break;

			case CASE_CUU:		// ESC [A cursor up, up arrow key.
				/* CUU */
				if ((row = param[0]) < 1)
					row = 1;
				fView->MoveCurUp(row);
				parsestate = groundtable;
				break;

			case CASE_CUD:		// ESC [B cursor down, down arrow key.
				/* CUD */
				if ((row = param[0]) < 1)
					row = 1;
				fView->MoveCurDown(row);
				parsestate = groundtable;
				break;

			case CASE_CUF:		// ESC [C cursor forword
				/* CUF */
				if ((row = param[0]) < 1)
					row = 1;
				fView->MoveCurRight(row);
				parsestate = groundtable;
				break;

			case CASE_CUB:		// ESC [D cursor backword
				/* CUB */
				if ((row = param[0]) < 1)
					row = 1;
				fView->MoveCurLeft(row);
				parsestate = groundtable;
				break;

			case CASE_CUP:		// ESC [...H move cursor
				/* CUP | HVP */
				if ((row = param[0]) < 1)
					row = 1;
				if (nparam < 2 || (col = param[1]) < 1)
					col = 1;

				fView->SetCurPos(col - 1, row - 1 );
				parsestate = groundtable;
				break;

			case CASE_ED:		// ESC [ ...J clear screen
				/* ED */
				switch (param[0]) {
					case DEFAULT:
					case 0:
						fView->EraseBelow();
						break;

					case 1:
						break;

					case 2:
						fView->SetCurPos(0, 0);
						fView->EraseBelow();
						break;
				}
				parsestate = groundtable;
				break;

			case CASE_EL:		// delete line
				/* EL */
				fView->DeleteColumns();
				parsestate = groundtable;
				break;

			case CASE_IL:
				/* IL */
				if ((row = param[0]) < 1)
					row = 1;
				fView->InsertNewLine(row);
				parsestate = groundtable;
				break;

			case CASE_DL:
				/* DL */
				if ((row = param[0]) < 1)
					row = 1;
				fView->DeleteLine(row);
				parsestate = groundtable;
				break;

			case CASE_DCH:
				/* DCH */
				if ((row = param[0]) < 1)
					row = 1;
				fView->DeleteChar(row);
				parsestate = groundtable;
				break;

			case CASE_SET:
				/* SET */
				fView->SetInsertMode(MODE_INSERT);
				parsestate = groundtable;
				break;

			case CASE_RST:
				/* RST */
				fView->SetInsertMode(MODE_OVER);
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
					fView->DeviceStatusReport(param[0]);
					parsestate = groundtable;
					break;

				case CASE_DECSTBM:
					/* DECSTBM - set scrolling region */

					if ((top = param[0]) < 1)
						top = 1;

					if (nparam < 2)
						bot = -1;
					else
						bot = param[1];

					top--;
					bot--;

					if (bot > top)
						fView->SetScrollRegion(top, bot);

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
					fView->SaveCursor();
					parsestate = groundtable;
					break;

				case CASE_DECRC:
					/* DECRC */
					fView->RestoreCursor();
					parsestate = groundtable;
					break;

				case CASE_HTS:
					/* HTS */
					//      TabSet(term->tabs, screen->cur_col);
					parsestate = groundtable;
					break;

				case CASE_RI:
					/* RI */
					fView->ScrollRegion(-1, -1, SCRDOWN, 1);
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
					parsestate = csitable;
					break;

				case CASE_OSC:
				{
					/* Operating System Command: ESC ] */
					char string[512];
					uint32 len = 0;
					uchar mode_char;
					GetReaderBuf(mode_char);
					if (mode_char != '0'
						&& mode_char != '1' 
						&& mode_char != '2') {
						parsestate = groundtable;
						break;
					}
					uchar current_char;
					GetReaderBuf(current_char);
					while (GetReaderBuf(current_char) == B_OK 
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
								fView->SetTitle(string);
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
					fView->SetCurY(row - 1);
					parsestate = groundtable;
					break;

				case CASE_HPA:		// ESC [...G move cursor absolute horizontal
					/* HPA (CH) */
					if ((col = param[0]) < 1)
						col = 1;

					// note beterm wants it 1-based unlike usual terminals
					fView->SetCurX(col - 1);
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
