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


#include "TermConst.h"

#include <Handler.h>
#include <OS.h>


#define READ_BUF_SIZE 2048
	// pty read buffer size
#define MIN_PTY_BUFFER_SPACE	16
	// minimal space left before the reader tries to read more
#define ESC_PARSER_BUFFER_SIZE	64
	// size of the parser buffer


class TerminalBuffer;

class TermParse : public BHandler {
public:
	TermParse(int fd);
	~TermParse();

	status_t StartThreads(TerminalBuffer *view);
	status_t StopThreads();

private:
	inline uchar _NextParseChar();

	// Initialize TermParse and PtyReader thread.
	status_t _InitTermParse();
	status_t _InitPtyReader();

	void _StopTermParse();
	void _StopPtyReader();

	int32 EscParse();
	int32 PtyReader();

	void DumpState(int *groundtable, int *parsestate, uchar c);

	static int32 _ptyreader_thread(void *);
	static int32 _escparse_thread(void *);

	status_t _ReadParserBuffer();

	void _DeviceStatusReport(int n);
	void _DecReqTermParms(int value);
	void _DecPrivateModeSet(int value);
	void _DecPrivateModeReset(int value);
	void _DecSaveCursor();
	void _DecRestoreCursor();
	int* _GuessGroundTable(int encoding);

	int fFd;

	uint32 fAttr;
	uint32 fSavedAttr;

	thread_id fParseThread;
	thread_id fReaderThread;
	sem_id fReaderSem;
	sem_id fReaderLocker;

	uint fBufferPosition;
	uchar fReadBuffer[READ_BUF_SIZE];
	vint32 fReadBufferSize;

	uchar fParserBuffer[ESC_PARSER_BUFFER_SIZE];
	int32 fParserBufferSize;
	int32 fParserBufferOffset;

	TerminalBuffer *fBuffer;

	bool fQuitting;
};

#endif // TERMPARSE_H
