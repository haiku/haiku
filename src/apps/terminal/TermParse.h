/*
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Kian Duffy, myob@users.sourceforge.net
 *		Siarzhuk Zharski, zharik@gmx.li
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
	int* _GuessGroundTable(int encoding);
	void _ProcessOperatingSystemControls(uchar* params);

	int fFd;

	thread_id fParseThread;
	thread_id fReaderThread;
	sem_id fReaderSem;
	sem_id fReaderLocker;

	uint fBufferPosition;
	uchar fReadBuffer[READ_BUF_SIZE];
	int32 fReadBufferSize;

	uchar fParserBuffer[ESC_PARSER_BUFFER_SIZE];
	int32 fParserBufferSize;
	int32 fParserBufferOffset;

	TerminalBuffer *fBuffer;

	bool fQuitting;
};

#endif // TERMPARSE_H
