/*
 * DbgMsg.cpp
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#include <cstdio>
#include <cstdarg>

#include <Message.h>
#include <File.h>
#include <Directory.h>
#include <fs_attr.h>

#include "DbgMsg.h"

#ifdef DBG


using namespace std;


void write_debug_stream(const char *format, ...)
{
	va_list	ap;
	va_start(ap, format);
	FILE *f = fopen("/boot/home/libprint.log", "aw+");
	vfprintf(f, format, ap);
	fclose(f);
	va_end(ap);
}


void DUMP_BFILE(BFile *in, const char *path)
{
	off_t size;
	if (B_NO_ERROR == in->GetSize(&size)) {
		uchar *buffer = new uchar[size];
		in->Read(buffer, size);
		BFile out(path, B_WRITE_ONLY | B_CREATE_FILE);
		out.Write(buffer, size);
		in->Seek(0, SEEK_SET);
		delete [] buffer;
	}
}


void DUMP_BMESSAGE(BMessage *msg)
{
	uint32 i;
	int32 j;
	char *name  = "";
	uint32 type = 0;
	int32 count = 0;

	DBGMSG(("\t************ START - DUMP BMessage ***********\n"));
	DBGMSG(("\taddress: %p\n", msg));
	if (!msg)
		return;	

//	DBGMSG(("\tmsg->what: 0x%x\n", msg->what));
	DBGMSG(("\tmsg->what: %c%c%c%c\n",
		*((char *)&msg->what + 3),
		*((char *)&msg->what + 2),
		*((char *)&msg->what + 1),
		*((char *)&msg->what + 0)));

	for (i= 0; msg->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; i++) {
		switch (type) {
		case B_BOOL_TYPE:
			for (j = 0; j < count; j++) {
				bool aBool;
				aBool = msg->FindBool(name, j);
				DBGMSG(("\t%s, B_BOOL_TYPE[%d]: %s\n",
					name, j, aBool ? "true" : "false"));
			}
			break;

		case B_INT8_TYPE:
			for (j = 0; j < count; j++) {
				int8 anInt8;
				msg->FindInt8(name, j, &anInt8);
				DBGMSG(("\t%s, B_INT8_TYPE[%d]: %d\n", name, j, (int)anInt8));
			}
			break;

		case B_INT16_TYPE:
			for (j = 0; j < count; j++) {
				int16 anInt16;
				msg->FindInt16(name, j, &anInt16);
				DBGMSG(("\t%s, B_INT16_TYPE[%d]: %d\n", name, j, (int)anInt16));
			}
			break;

		case B_INT32_TYPE:
			for (j = 0; j < count; j++) {
				int32 anInt32;
				msg->FindInt32(name, j, &anInt32);
				DBGMSG(("\t%s, B_INT32_TYPE[%d]: %d\n", name, j, (int)anInt32));
			}
			break;

		case B_INT64_TYPE:
			for (j = 0; j < count; j++) {
				int64 anInt64;
				msg->FindInt64(name, j, &anInt64);
				DBGMSG(("\t%s, B_INT64_TYPE[%d]: %d\n", name, j, (int)anInt64));
			}
			break;

		case B_FLOAT_TYPE:
			for (j = 0; j < count; j++) {
				float aFloat;
				msg->FindFloat(name, j, &aFloat);
				DBGMSG(("\t%s, B_FLOAT_TYPE[%d]: %f\n", name, j, aFloat));
			}
			break;

		case B_DOUBLE_TYPE:
			for (j = 0; j < count; j++) {
				double aDouble;
				msg->FindDouble(name, j, &aDouble);
				DBGMSG(("\t%s, B_DOUBLE_TYPE[%d]: %f\n", name, j, (float)aDouble));
			}
			break;

		case B_STRING_TYPE:
			for (j = 0; j < count; j++) {
				const char *string;
				msg->FindString(name, j, &string);
				DBGMSG(("\t%s, B_STRING_TYPE[%d]: %s\n", name, j, string));
			}
			break;

		case B_POINT_TYPE:
			for (j = 0; j < count; j++) {
				BPoint aPoint;
				msg->FindPoint(name, j, &aPoint);
				DBGMSG(("\t%s, B_POINT_TYPE[%d]: %f, %f\n",
					name, j, aPoint.x, aPoint.y));
			}
			break;

		case B_RECT_TYPE:
			for (j = 0; j < count; j++) {
				BRect aRect;
				msg->FindRect(name, j, &aRect);
				DBGMSG(("\t%s, B_RECT_TYPE[%d]: %f, %f, %f, %f\n",
					name, j, aRect.left, aRect.top, aRect.right, aRect.bottom));
			}
			break;

		case B_REF_TYPE:
		case B_MESSAGE_TYPE:
		case B_MESSENGER_TYPE:
		case B_POINTER_TYPE:
			DBGMSG(("\t%s, 0x%x, count: %d\n",
				name ? name : "(null)", type, count));
			break;
		default:
			DBGMSG(("\t%s, 0x%x, count: %d\n",
				name ? name : "(null)", type, count));
			break;
		}

		name  = "";
		type  = 0;
		count = 0;
	}
	DBGMSG(("\t************ END - DUMP BMessage ***********\n"));
}

#define PD_DRIVER_NAME		"Driver Name"
#define PD_PRINTER_NAME		"Printer Name"
#define PD_COMMENTS			"Comments"

void DUMP_BDIRECTORY(BDirectory *dir)
{
	DUMP_BNODE(dir);
}

void DUMP_BNODE(BNode *dir)
{
	char buffer1[256];
	char buffer2[256];
	attr_info info;
	int32 i;
	float f;
	BRect rc;
	bool b;

	DBGMSG(("\t************ STRAT - DUMP BNode ***********\n"));

	dir->RewindAttrs();
	while (dir->GetNextAttrName(buffer1) == B_NO_ERROR) {
		dir->GetAttrInfo(buffer1, &info);
		switch (info.type) {
		case B_ASCII_TYPE:
			dir->ReadAttr(buffer1, info.type, 0, buffer2, sizeof(buffer2));
			DBGMSG(("\t%s, B_ASCII_TYPE: %s\n", buffer1, buffer2));
			break;
		case B_STRING_TYPE:
			dir->ReadAttr(buffer1, info.type, 0, buffer2, sizeof(buffer2));
			DBGMSG(("\t%s, B_STRING_TYPE: %s\n", buffer1, buffer2));
			break;
		case B_INT32_TYPE:
			dir->ReadAttr(buffer1, info.type, 0, &i, sizeof(i));
			DBGMSG(("\t%s, B_INT32_TYPE: %d\n", buffer1, i));
			break;
		case B_FLOAT_TYPE:
			dir->ReadAttr(buffer1, info.type, 0, &f, sizeof(f));
			DBGMSG(("\t%s, B_FLOAT_TYPE: %f\n", buffer1, f));
			break;
		case B_RECT_TYPE:
			dir->ReadAttr(buffer1, info.type, 0, &rc, sizeof(rc));
			DBGMSG(("\t%s, B_RECT_TYPE: %f, %f, %f, %f\n", buffer1, rc.left, rc.top, rc.right, rc.bottom));
			break;
		case B_BOOL_TYPE:
			dir->ReadAttr(buffer1, info.type, 0, &b, sizeof(b));
			DBGMSG(("\t%s, B_BOOL_TYPE: %d\n", buffer1, (int)b));
			break;
		default:
			DBGMSG(("\t%s, %c%c%c%c\n",
				buffer1,
				*((char *)&info.type + 3),
				*((char *)&info.type + 2),
				*((char *)&info.type + 1),
				*((char *)&info.type + 0)));
			break;
		}
	}

	DBGMSG(("\t************ END - DUMP BNode ***********\n"));
}

#endif	/* DBG */
