/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 */


#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <app/Application.h>
#include <support/Beep.h>
#include <Message.h>
#include <MessageRunner.h>


#include "TermParse.h"
#include "TermView.h"
#include "VTparse.h"
#include "TermConst.h"
#include "CodeConv.h"

extern int gPfd; // defined Muterminal.cpp

/////////////////////////////////////////////////////////////////////////////
// PtyReader ... Get character from pty device.
//
/////////////////////////////////////////////////////////////////////////////
int32
TermParse::PtyReader(void *data)
{
  int nread;
  uint read_p = 0;

  TermParse *theObj = (TermParse *) data;
  
  uchar buf [READ_BUF_SIZE];

  while (theObj->fQuitting) {
    /*
     * If Pty Buffer nearly full, snooze this thread, and continue.
     */
    if ((read_p - theObj->fParser_p) > READ_BUF_SIZE - 16) {
      theObj->fLockFlag = READ_BUF_SIZE / 2;
      acquire_sem (theObj->fReaderLocker);
    }
    
    /*
     * Read PTY.
     */
    nread = read (gPfd, buf,
		  READ_BUF_SIZE - (read_p - theObj->fParser_p));
    if (nread <= 0) {
      be_app->PostMessage(B_QUIT_REQUESTED);
      exit_thread (B_ERROR);
      
    }
    

    int left = READ_BUF_SIZE - (read_p % READ_BUF_SIZE);
    int mod = read_p % READ_BUF_SIZE;
    
    /*
     * Copy read string to PtyBuffer.
     */
    if (nread >= left) {
      memcpy (theObj->fReadBuf + mod, buf, left);
      memcpy (theObj->fReadBuf, buf + left, nread - left);
    }
    else {
      memcpy (theObj->fReadBuf + mod, buf, nread);
    }
    read_p += nread;
    
    /*
     * Release semaphore. Number of semaphore counter is nread.
     */
    release_sem_etc (theObj->fReaderSem, nread, 0);

  }
  theObj->fReaderThread = -1;
  exit_thread (B_OK);
  return B_OK;
}
///////////////////////////////////////////////////////////////////////////
// GetReaderBuf ... Get char pty reader buffer.
//
///////////////////////////////////////////////////////////////////////////

uchar
TermParse::GetReaderBuf (void)
{
  int c;

  switch (acquire_sem_etc (fReaderSem, 1, B_TIMEOUT, (bigtime_t)10000.0)) {
    case B_NO_ERROR:
      break;
    case B_TIMED_OUT:
    default:
      fViewObj->ScrollAtCursor();
      fViewObj->UpdateLine();

      // Reset cursor blinking time and turn on cursor blinking.
      fCursorUpdate->SetInterval (1000000);
      fViewObj->SetCurDraw (CURON);

      // wait new input from pty.
      acquire_sem (fReaderSem);
      break;
  }

  c = fReadBuf[fParser_p % READ_BUF_SIZE];
  fParser_p++;
  /*
   * If PtyReader thread locked, decliment counter and unlock thread.
   */
  if (fLockFlag != 0) {
    fLockFlag--;
    if (fLockFlag == 0) {
      release_sem (fReaderLocker);
    }
  }
  
  fViewObj->SetCurDraw (CUROFF);
  
  return c;
}

///////////////////////////////////////////////////////////////////////////
// InitTermParse ... Initialize and spawn EscParse thread.
//
///////////////////////////////////////////////////////////////////////////
status_t
TermParse::InitTermParse (TermView *inViewObj, CodeConv *inConvObj)
{

  fViewObj = inViewObj;
  fConvObj = inConvObj;

  fCursorUpdate = new BMessageRunner (BMessenger (fViewObj),
				      new BMessage (MSGRUN_CURSOR),
				      1000000);
  
  if (fParseThread < 0)
    fParseThread =
      spawn_thread (EscParse, "EscParse", B_DISPLAY_PRIORITY, this);
  else
    return B_BAD_THREAD_ID;
  
  return (resume_thread ( fParseThread));

}
///////////////////////////////////////////////////////////////////////////
// InitPtyReader ... Initialize and spawn PtyReader thread.
//
///////////////////////////////////////////////////////////////////////////
thread_id
TermParse::InitPtyReader (TermWindow *inWinObj)
{

  fWinObj = inWinObj;
  
  if (fReaderThread < 0)
    fReaderThread =
      spawn_thread (PtyReader, "PtyReader", B_NORMAL_PRIORITY, this);
  else
    return B_BAD_THREAD_ID;

  fReaderSem = create_sem (0, "pty_reader_sem");
  fReaderLocker = create_sem (0, "pty_locker_sem");
  
  resume_thread (fReaderThread);
  
  return (fReaderThread);
}

/*
 *  Constructer and Destructer.
 */

TermParse::TermParse (void)
{

  fParseThread = -1;
  fReaderThread = -1;
  fLockFlag = 0;
  fParser_p = 0;
  fQuitting = 1;

}

TermParse::~TermParse (void)
{
  //status_t sts;

  fQuitting = 0;
  kill_thread(fParseThread);

  kill_thread(fReaderThread);
  delete_sem (fReaderSem);
  delete_sem (fReaderLocker);

}

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


/* MuTerminal coding system (global varriable) */
int gNowCoding = M_UTF8;

#define DEFAULT -1
#define NPARAM 10		// Max parameters


int32
TermParse::EscParse(void *data)
{
	int tmp;
	int top, bot;
	int cs96;
	uchar curess = 0;

	TermParse *theObj = (TermParse *)data;

	TermView *viewObj = theObj->fViewObj;
	CodeConv *convObj = theObj->fConvObj;

	uchar cbuf[4], dstbuf[4];
	uchar *ptr;

	int *parsestate, *groundtable;
	int now_coding = -1;

	uchar c;
	ushort attr = BACKCOLOR;

	int param[NPARAM];
	int nparam = 1;

	int row, col;
	int width;

	/* default coding system is UTF8 */
	groundtable = utf8_groundtable;
	parsestate = groundtable;

	while (theObj->fQuitting) {
		c = theObj->GetReaderBuf();

		if (now_coding != gNowCoding) {
			/*
			 * Change coding, change parse table.
			 */
			switch (gNowCoding) {
				case M_UTF8:
					groundtable = utf8_groundtable;
					break;
				case M_ISO_8859_1:
				case M_ISO_8859_2:
				case M_ISO_8859_3:
				case M_ISO_8859_4:
				case M_ISO_8859_5:
				case M_ISO_8859_6:
				case M_ISO_8859_7:
				case M_ISO_8859_8:
				case M_ISO_8859_9:
				case M_ISO_8859_10:
					groundtable = iso8859_groundtable;
					break;
				case M_SJIS:
					groundtable = sjis_groundtable;
					break;
				case M_EUC_JP:
				case M_EUC_KR:
				case M_ISO_2022_JP:
					groundtable = iso8859_groundtable;
					break;
			}
			parsestate = groundtable;
			now_coding = gNowCoding;
    		}

		switch (parsestate[c]) {
			case CASE_PRINT:
				cbuf[0] = c;
				cbuf[1] = '\0';
				width = HALF_WIDTH;
				viewObj->PutChar(cbuf, attr, width);
				break;

			case CASE_PRINT_GR:
				/* case iso8859 gr character, or euc */
				ptr = cbuf;
				if (now_coding == M_EUC_JP || now_coding == M_EUC_KR
					|| now_coding == M_ISO_2022_JP) {
					switch (parsestate[curess]) {
						case CASE_SS2:		/* JIS X 0201 */
							*ptr++ = curess;
							*ptr++ = c;
							*ptr = 0;
							width = 1;
							curess = 0;
							break;

						case CASE_SS3:		/* JIS X 0212 */
							*ptr++ = curess;
							*ptr++ = c;
							*ptr++ = theObj->GetReaderBuf ();
							*ptr = 0;
							width = 2;
							curess = 0;
							break;

						default:		/* JIS X 0208 */
							*ptr++ = c;
							*ptr++ = theObj->GetReaderBuf ();
							*ptr = 0;
							width = 2;
							break;
					}
				} else {
					/* ISO-8859-1...10 and MacRoman */
					*ptr++ = c;
					*ptr = 0;
					width = 1;
				}

				if (now_coding != M_ISO_2022_JP)
					convObj->ConvertToInternal((char*)cbuf, -1, (char*)dstbuf, now_coding);
				else
					convObj->ConvertToInternal((char*)cbuf, -1, (char*)dstbuf, M_EUC_JP);

				viewObj->PutChar(dstbuf, attr, width);
				break;

			case CASE_PRINT_CS96:
				cbuf[0] = c | 0x80;
				cbuf[1] = theObj->GetReaderBuf() | 0x80;
				cbuf[2] = 0;
				width = 2;
				convObj->ConvertToInternal((char*)cbuf, 2, (char*)dstbuf, M_EUC_JP);
				viewObj->PutChar(dstbuf, attr, width);
				break;

			case CASE_LF:
				viewObj->PutLF();
				break;

			case CASE_CR:
				viewObj->PutCR();
				break;

			case CASE_SJIS_KANA:
				cbuf[0] = (uchar)c;
				cbuf[1] = '\0';
				convObj->ConvertToInternal((char*)cbuf, 1, (char*)dstbuf, now_coding);
				width = 1;
				viewObj->PutChar(dstbuf, attr, width);
				break;

			case CASE_SJIS_INSTRING:
				cbuf[0] = (uchar)c;
				cbuf[1] = theObj->GetReaderBuf();
				cbuf[2] = '\0';
				convObj->ConvertToInternal((char*)cbuf, 2, (char*)dstbuf, now_coding);
				width = 2;
				viewObj->PutChar(dstbuf, attr, width);
				break;

			case CASE_UTF8_2BYTE:
				cbuf[0] = (uchar)c;
				c = theObj->GetReaderBuf();
				if (groundtable[c] != CASE_UTF8_INSTRING)
					break;
				cbuf[1] = (uchar)c;
				cbuf[2] = '\0';
				width = convObj->UTF8GetFontWidth((char*)cbuf);
				viewObj->PutChar(cbuf, attr, width);
				break;

			case CASE_UTF8_3BYTE:
				cbuf[0] = c;
				c = theObj->GetReaderBuf();
				if (groundtable[c] != CASE_UTF8_INSTRING)
					break;
				cbuf[1] = c;

				c = theObj->GetReaderBuf();
				if (groundtable[c] != CASE_UTF8_INSTRING)
					break;
				cbuf[2] = c;
				cbuf[3] = '\0';
				width = convObj->UTF8GetFontWidth((char*)cbuf);
				viewObj->PutChar (cbuf, attr, width);
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
				cs96 = 0;
				theObj->GetReaderBuf ();
				parsestate = groundtable;
				break;

			case CASE_GROUND_STATE:
				/* exit ignore mode */
				parsestate = groundtable;
				break;

			case CASE_BELL:
				beep();
				break;

			case CASE_BS:
				viewObj->MoveCurLeft(1);
				break;

			case CASE_TAB:
				tmp = viewObj->GetCurX();
				tmp %= 8;
				viewObj->MoveCurRight(8 - tmp);
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
				viewObj->InsertSpace(row);
				parsestate = groundtable;
				break;

			case CASE_CUU:		// ESC [A cursor up, up arrow key.
				/* CUU */
				if ((row = param[0]) < 1)
					row = 1;
				viewObj->MoveCurUp(row);
				parsestate = groundtable;
				break;

			case CASE_CUD:		// ESC [B cursor down, down arrow key.
				/* CUD */
				if ((row = param[0]) < 1)
					row = 1;
				viewObj->MoveCurDown(row);
				parsestate = groundtable;
				break;

			case CASE_CUF:		// ESC [C cursor forword
				/* CUF */
				if ((row = param[0]) < 1)
					row = 1;
				viewObj->MoveCurRight(row);
				parsestate = groundtable;
				break;

			case CASE_CUB:		// ESC [D cursor backword
				/* CUB */
				if ((row = param[0]) < 1)
					row = 1;
				viewObj->MoveCurLeft(row);
				parsestate = groundtable;
				break;

			case CASE_CUP:		// ESC [...H move cursor
				/* CUP | HVP */
				if ((row = param[0]) < 1)
					row = 1;
				if (nparam < 2 || (col = param[1]) < 1)
					col = 1;

				viewObj->SetCurPos(col - 1, row - 1 );
				parsestate = groundtable;
				break;

			case CASE_ED:		// ESC [ ...J clear screen
				/* ED */
				switch (param[0]) {
					case DEFAULT:
					case 0:
						viewObj->EraseBelow();
						break;

					case 1:
						break;

					case 2:
						viewObj->SetCurPos(0, 0);
						viewObj->EraseBelow();
						break;
				}
				parsestate = groundtable;
				break;

			case CASE_EL:		// delete line
				/* EL */
				viewObj->DeleteColumns();
				parsestate = groundtable;
				break;

			case CASE_IL:
				/* IL */
				if ((row = param[0]) < 1)
					row = 1;
				viewObj->PutNL(row);
				parsestate = groundtable;
				break;

			case CASE_DL:
				/* DL */
				if ((row = param[0]) < 1)
					row = 1;
				viewObj->DeleteLine(row);
				parsestate = groundtable;
				break;

			case CASE_DCH:
				/* DCH */
				if ((row = param[0]) < 1)
					row = 1;
				viewObj->DeleteChar(row);
				parsestate = groundtable;
				break;

			case CASE_SET:
				/* SET */
				viewObj->SetInsertMode(MODE_INSERT);
				parsestate = groundtable;
				break;

			case CASE_RST:
				/* RST */
				viewObj->SetInsertMode(MODE_OVER);
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
					viewObj->DeviceStatusReport(param[0]);
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
						viewObj->SetScrollRegion(top, bot);

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
					viewObj->SaveCursor();
					parsestate = groundtable;
					break;

				case CASE_DECRC:
					/* DECRC */
					viewObj->RestoreCursor();
					parsestate = groundtable;
					break;

				case CASE_HTS:
					/* HTS */
					//      TabSet(term->tabs, screen->cur_col);
					parsestate = groundtable;
					break;

				case CASE_RI:
					/* RI */
					viewObj->ScrollRegion(-1, -1, SCRDOWN, 1);
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
					char mode_char = theObj->GetReaderBuf();
					if (mode_char != '0'
						&& mode_char != '1' 
						&& mode_char != '2') {
						parsestate = groundtable;
						break;
					}
					char current_char = theObj->GetReaderBuf();
					while ((current_char = theObj->GetReaderBuf()) != 0x7) {
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
								theObj->fWinObj->SetTitle(string);
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

				default:
					break;
		}
	}
	theObj->fParseThread = -1;
	exit_thread(B_OK);
	return B_OK;
}

