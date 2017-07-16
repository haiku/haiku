/*
 * DbgMsg.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __DBGMSG_H
#define __DBGMSG_H

#include <Directory.h>
#include <File.h>
#include <Message.h>
#include <Node.h>

#include <stdio.h>

// #define DBG

#ifdef DBG
	void write_debug_stream(const char *, ...)  __PRINTFLIKE(1,2);
	void DUMP_BFILE(BFile *file, const char *name);
	void DUMP_BMESSAGE(BMessage *msg);
	void DUMP_BDIRECTORY(BDirectory *dir);
	void DUMP_BNODE(BNode *node);
	#define DBGMSG(args)	write_debug_stream args
#else
	#define DUMP_BFILE(file, name)	(void)0
	#define DUMP_BMESSAGE(msg)		(void)0
	#define DUMP_BDIRECTORY(dir)	(void)0
	#define DUMP_BNODE(node)		(void)0
	#define DBGMSG(args)			(void)0
#endif

#endif	/* __DBGMSG_H */
