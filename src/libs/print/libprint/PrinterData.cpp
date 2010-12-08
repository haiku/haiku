/*
 * PrinterData.cpp
 * Copyright 1999-2000 Y.Takagi All Rights Reserved.
 */

#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>
#include <StorageDefs.h>

#include "PrinterData.h"

const char *PD_DRIVER_NAME      = "Driver Name";
const char *PD_PRINTER_NAME     = "Printer Name";
const char *PD_COMMENTS         = "Comments";
const char *PD_TRANSPORT        = "transport";
const char *PD_PROTOCOL_CLASS   = "libprint:protocolClass";


PrinterData::PrinterData(BNode *node)
	:
	fNode(node),
	fProtocolClass(0)
{
}


PrinterData::~PrinterData()
{
}


void
PrinterData::Load()
{
	if (fNode == NULL) return;
	
	char buffer[512];

	fNode->ReadAttr(PD_DRIVER_NAME,  B_STRING_TYPE, 0, buffer, sizeof(buffer));
	// TODO fix possible buffer overrun (not terminated string).
	fDriverName = buffer;
	fNode->ReadAttr(PD_PRINTER_NAME, B_STRING_TYPE, 0, buffer, sizeof(buffer));
	fPrinterName = buffer;
	fNode->ReadAttr(PD_COMMENTS,     B_STRING_TYPE, 0, buffer, sizeof(buffer));
	fComments = buffer;
	fNode->ReadAttr(PD_TRANSPORT,    B_STRING_TYPE, 0, buffer, sizeof(buffer));
	fTransport = buffer;
	
	int32 valueI32;
	fNode->ReadAttr(PD_PROTOCOL_CLASS, B_INT32_TYPE, 0, &valueI32, sizeof(valueI32));
	fProtocolClass = (int)valueI32;
}


void
PrinterData::Save()
{
	if (fNode == NULL)
		return;

	int32 valueI32 = (int32)fProtocolClass;
	fNode->WriteAttr(PD_PROTOCOL_CLASS, B_INT32_TYPE, 0, &valueI32,
		sizeof(valueI32));
}


bool
PrinterData::GetPath(string &path) const
{
	if (fNode == NULL)
		return false;
	
	node_ref nref;
	if (fNode->GetNodeRef(&nref) != B_OK)
		return false;

	BDirectory dir(&nref);
	if (dir.InitCheck() != B_OK)
		return false;
		
	BPath path0(&dir, ".");
	if (path0.InitCheck() != B_OK)
		return false;
		
	path = path0.Path();
	return true;
}
