/*
 * PCL5Entry.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 * Copyright 2003 Michael Pfeiffer.
 */

#include <File.h>
#include <Message.h>
#include <Node.h>

#include "Exports.h"
#include "PCL5.h"
#include "PrinterData.h"
#include "PCL5Cap.h"
#include "UIDriver.h"
#include "AboutBox.h"
#include "DbgMsg.h"

char *add_printer(char *printer_name)
{
	DBGMSG((">PCL5: add_printer\n"));
	DBGMSG(("\tprinter_name: %s\n", printer_name));
	DBGMSG(("<PCL5: add_printer\n"));
	return printer_name;
}

BMessage *config_page(BNode *node, BMessage *msg)
{
	DBGMSG((">PCL5: config_page\n"));
	DUMP_BMESSAGE(msg);
	DUMP_BNODE(node);

	PrinterData printer_data(node);
	PCL5Cap printer_cap(&printer_data);
	UIDriver drv(msg, &printer_data, &printer_cap);
	BMessage *result = drv.configPage();

	DUMP_BMESSAGE(result);
	DBGMSG(("<PCL5: config_page\n"));
	return result;
}

BMessage *config_job(BNode *node, BMessage *msg)
{
	DBGMSG((">PCL5: config_job\n"));
	DUMP_BMESSAGE(msg);
	DUMP_BNODE(node);

	PrinterData printer_data(node);
	PCL5Cap printer_cap(&printer_data);
	UIDriver drv(msg, &printer_data, &printer_cap);
	BMessage *result = drv.configJob();

	DUMP_BMESSAGE(result);
	DBGMSG(("<PCL5: config_job\n"));
	return result;
}

BMessage *take_job(BFile *spool, BNode *node, BMessage *msg)
{
	DBGMSG((">PCL5: take_job\n"));
	DUMP_BMESSAGE(msg);
	DUMP_BNODE(node);

	PrinterData printer_data(node);
	PCL5Cap printer_cap(&printer_data);
	PCL5Driver drv(msg, &printer_data, &printer_cap);
	BMessage *result = drv.takeJob(spool);

//	DUMP_BMESSAGE(result);
	DBGMSG(("<PCL5: take_job\n"));
	return result;
}

int main()
{
	AboutBox app("application/x-vnd.PCL5-compatible", "PCL5 Compatible", "0.1", 
	"libprint Copyright © 1999-2000 Y.Takagi\n"
	"PCL5 driver Copyright © 2003 Michael Pfeiffer.\n"
	"All Rights Reserved.");
	app.Run();
	return 0;
}
