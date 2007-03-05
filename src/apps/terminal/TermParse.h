/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef TERMPARSE_H
#define TERMPARSE_H

#include <OS.h>
#include <MessageRunner.h>

#include "TermConst.h"

class TermView;
class CodeConv;
class TermWindow;
 
//PtyReader buffer size.
#define READ_BUF_SIZE 2048

class TermParse : public BHandler {
public:
	TermParse(int fd, TermWindow *inWinObj, TermView *inViewObj, CodeConv *inConvObj);
	~TermParse();

	status_t StartThreads();
  
private:
	// Initialize TermParse and PtyReader thread.
	status_t InitTermParse();
	status_t InitPtyReader();

	// Delete TermParse and PtyReader thread.
	status_t AbortTermParse();
	status_t AbortPtyReader();

	int32 EscParse();
	int32 PtyReader();

	static int32 _ptyreader_thread(void *);
	static int32 _escparse_thread(void *);

	// Reading ReadBuf at one Char.
	status_t GetReaderBuf(uchar &c);
  
	int fFd;

	TermView *fViewObj;
	TermWindow *fWinObj;
	CodeConv *fConvObj;
  
	thread_id fParseThread;
	sem_id fParseSem;

	thread_id fReaderThread;
	sem_id fReaderSem;
	sem_id fReaderLocker;

	BMessageRunner *fCursorUpdate;

	uint fParser_p;	/* EscParse reading buffer pointer */
	int fLockFlag;	/* PtyReader lock flag */

	bool fQuitting;
  
	uchar fReadBuf[READ_BUF_SIZE]; /* Reading buffer */
};

#endif // TERMPARSE_H
