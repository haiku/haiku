/*
 * Lips3Entry.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <File.h>
#include <Message.h>
#include <Node.h>

#include "Exports.h"
#include "Lips3.h"
#include "PrinterData.h"
#include "Lips3Cap.h"
#include "UIDriver.h"
#include "AboutBox.h"
#include "DbgMsg.h"

char *add_printer(char *printer_name)
{
	DBGMSG((">LIPS3: add_printer\n"));
	DBGMSG(("\tprinter_name: %s\n", printer_name));
	DBGMSG(("<LIPS3: add_printer\n"));
	return printer_name;
}

BMessage *config_page(BNode *node, BMessage *msg)
{
	DBGMSG((">LIPS3: config_page\n"));
	DUMP_BMESSAGE(msg);
	DUMP_BNODE(node);

	PrinterData printer_data(node);
	Lips3Cap printer_cap(&printer_data);
	UIDriver drv(msg, &printer_data, &printer_cap);
	BMessage *result = drv.configPage();

	DUMP_BMESSAGE(result);
	DBGMSG(("<LIPS3: config_page\n"));
	return result;
}

BMessage *config_job(BNode *node, BMessage *msg)
{
	DBGMSG((">LIPS3: config_job\n"));
	DUMP_BMESSAGE(msg);
	DUMP_BNODE(node);

	PrinterData printer_data(node);
	Lips3Cap printer_cap(&printer_data);
	UIDriver drv(msg, &printer_data, &printer_cap);
	BMessage *result = drv.configJob();

	DUMP_BMESSAGE(result);
	DBGMSG(("<LIPS3: config_job\n"));
	return result;
}

BMessage *take_job(BFile *spool, BNode *node, BMessage *msg)
{
	DBGMSG((">LIPS3: take_job\n"));
	DUMP_BMESSAGE(msg);
	DUMP_BNODE(node);

	PrinterData printer_data(node);
	Lips3Cap printer_cap(&printer_data);
	LIPS3Driver drv(msg, &printer_data, &printer_cap);
	BMessage *result = drv.takeJob(spool);

//	DUMP_BMESSAGE(result);
	DBGMSG(("<LIPS3: take_job\n"));
	return result;
}

int main()
{
	AboutBox app("application/x-vnd.lips3-compatible", "Canon LIPS3 Compatible", "0.9.4", "Copyright © 1999-2000 Y.Takagi. All Rights Reserved.");
	app.Run();
	return 0;
}
