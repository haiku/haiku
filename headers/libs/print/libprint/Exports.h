/*
 * Exports.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __EXPORTS_H
#define __EXPORTS_H

#include <BeBuild.h>

class BNode;
class BMessage;
class BFile;

extern "C" {
	_EXPORT char *add_printer(char *printer_name);
	_EXPORT BMessage *config_page(BNode *node, BMessage *msg);
	_EXPORT BMessage *config_job(BNode *node, BMessage *msg);
	_EXPORT BMessage *take_job(BFile *spool_file, BNode *node, BMessage *msg);
}

#endif	// __EXPORTS_H
