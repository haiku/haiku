#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <BeBuild.h>
#include <Font.h>
#include <Message.h>
#include <String.h>
#include "DumpMessage.h"

//#define WHAT_ALWAYS_HEX 1

const char *msg_header_comment = "// new BMessage\n"; // avoids mime to think it's a .bmp ("BM")

// '_' also widely used in what codes
inline int myisprint(int c)
{
	if (isalnum(c))
		return 1;
	return (c == '_')?1:0;
}

status_t HexDumpToStream(const void *data, size_t len, BDataIO &stream, const char *prefix = NULL)
{
	const unsigned char *p = (unsigned char *)data;
	char buffer[100];
	size_t i, j;
	for (i=0; i<len; i+=16) {
		if (prefix) stream.Write(prefix, strlen(prefix));
		sprintf(buffer, "0x%06lx: ", i);
		stream.Write(buffer, strlen(buffer));
		for (j=0; j<16; j++) {
			if (i+j < len)
				sprintf(buffer, "%02x", p[i+j]);
			else
				sprintf(buffer, "  ");
			if (j % 4 == 3)
				sprintf(buffer+strlen(buffer), " ");
			stream.Write(buffer, strlen(buffer));
		}
		sprintf(buffer, " '");
		stream.Write(buffer, strlen(buffer));
		for (j=0; j<16; j++) {
			if (i+j >= len)
				sprintf(buffer, " ");
			//else if (p[i+j] < 255 && p[i+j] >= 0x20)
			else if (isalpha(p[i+j]))
				sprintf(buffer, "%c", p[i+j]);
			else
				sprintf(buffer, ".");
			stream.Write(buffer, 1);
		}
		sprintf(buffer, "'\n");
		stream.Write(buffer, strlen(buffer));
	}
	return B_OK;
}

/* look up human readable names from an other BMessage */
bool LookUpFieldName(const char **name, const char *field_name, BMessage *names)
{
	if (names == NULL)
		return false;
	if (names->FindString(field_name, name) == B_OK)
		return true;
	return false;
}

status_t DumpMessageToStream(BMessage *message, BDataIO &stream, int tabCount, BMessage *names)
{
	int32 index;
	void *cookie = NULL;
	const char *field_name;
	type_code field_code;
	int32 field_count;
	char buffer[80];
	char tabs[20];
	const char *easy_name;

	if (message == NULL)
		return EINVAL;

	if (tabCount < 1)
		stream.Write(msg_header_comment, strlen(msg_header_comment));

	memset(tabs, '\t', (++tabCount) + 1);
	tabs[tabCount+1] = '\0';
	//tabCount;

#ifndef WHAT_ALWAYS_HEX
	if (	myisprint(message->what & 0x0ff) &&
		myisprint((message->what >> 8) & 0x0ff) &&
		myisprint((message->what >> 16) & 0x0ff) &&
		myisprint((message->what >> 24) & 0x0ff))
		sprintf(buffer, "BMessage('%c%c%c%c') {\n", 
			(char)(message->what >> 24) & 0x0ff,
			(char)(message->what >> 16) & 0x0ff,
			(char)(message->what >> 8) & 0x0ff,
			(char)message->what & 0x0ff);
	else
#endif
		sprintf(buffer, "BMessage(0x%08lx) {\n", message->what);
//	stream.Write(tabs+2, tabCount-2);
	stream.Write(buffer, strlen(buffer));

#ifdef B_BEOS_VERSION_DANO
	while (message->GetNextName(&cookie, 
				&field_name, 
				&field_code, 
				&field_count) == B_OK) {
#else
#warning mem leak likely! (name=char *)
	for (int which=0; message->GetInfo(B_ANY_TYPE, which, 
			(char **)&field_name, &field_code, &field_count) == B_OK; which++) {
#endif
		if (LookUpFieldName(&easy_name, field_name, names)) {
			stream.Write(tabs+1, tabCount);
			stream.Write("// ", 3);
			stream.Write(easy_name, strlen(easy_name));
			stream.Write("\n", 1);
		}

		for (index=0; index < field_count; index++) {
			stream.Write(tabs+1, tabCount);
			stream.Write(field_name, strlen(field_name));
			if (field_count > 1) {
				sprintf(buffer, "[%ld]", index);
				stream.Write(buffer, strlen(buffer));
			}
			stream.Write(" = ", 3);
		
			switch (field_code) {
			case 'MSGG':
				{
					BMessage m;
					if (message->FindMessage(field_name, index, &m) >= B_OK)
						DumpMessageToStream(&m, stream, tabCount, names);
				}
				break;
#ifdef B_BEOS_VERSION_DANO
			case 'FONt':
				{
					BFont f;
					if (message->FindFlat(field_name, index, &f) >= B_OK)
						stream << f;
					stream.Write("\n", 1);
				}
				break;
			case B_RGB_COLOR_TYPE:
				{
					rgb_color c;
					if (message->FindRGBColor(field_name, index, &c) >= B_OK) {
						sprintf(buffer, "rgb_color(%d,%d,%d,%d)", 
							c.red, c.green, c.blue, c.alpha);
						stream.Write(buffer, strlen(buffer));
					}
					stream.Write("\n", 1);
				}
				break;
#else
#warning IMPLEMENT ME
#endif
			case B_BOOL_TYPE:
				{
					bool value;
					if (message->FindBool(field_name, index, &value) >= B_OK) {
						sprintf(buffer, "bool(%s)", value?"true":"false");
						stream.Write(buffer, strlen(buffer));
					}
					stream.Write("\n", 1);
				}
				break;
			case B_INT32_TYPE:
				{
					int32 value;
					if (message->FindInt32(field_name, index, &value) >= B_OK) {
#if 1
						if (value == 0)
							sprintf(buffer, "int32(0 or (nil))");
						else
#endif
//							sprintf(buffer, "int32(%d)", value);
							sprintf(buffer, "int32(%ld or 0x%lx)", value, value);
						stream.Write(buffer, strlen(buffer));
					}
					stream.Write("\n", 1);
				}
				break;
			case B_FLOAT_TYPE:
				{
					float value;
					if (message->FindFloat(field_name, index, &value) >= B_OK) {
							sprintf(buffer, "float(%f)", value);
						stream.Write(buffer, strlen(buffer));
					}
					stream.Write("\n", 1);
				}
				break;
			case B_STRING_TYPE:
				{
					const char *value;
					if (message->FindString(field_name, index, &value) >= B_OK) {
						BString str(value);
						str.CharacterEscape("\\\"\n", '\\');
						//sprintf(buffer, "string(\"%s\", %ld bytes)", str.String(), strlen(value));
						// DO NOT use buffer!
						str.Prepend("string(\"");
						str << "\", " << strlen(value) << " bytes)";
						stream.Write(str.String(), strlen(str.String()));
					}
					stream.Write("\n", 1);
				}
				break;
			case B_POINT_TYPE:
				{
					BPoint value;
					if (message->FindPoint(field_name, index, &value) >= B_OK) {
						sprintf(buffer, "BPoint(%1.1f, %1.1f)", value.x, value.y);
						stream.Write(buffer, strlen(buffer));
					}
					stream.Write("\n", 1);
				}
				break;
			default:
				{
					const void *data;
					ssize_t numBytes = 0;
					if (message->FindData(field_name, field_code, index, &data, &numBytes) != B_OK) {
						//stream.Write("\n", 1);
						break;
					}
					
					if (	isalnum(field_code & 0x0ff) &&
						isalnum((field_code >> 8) & 0x0ff) &&
						isalnum((field_code >> 16) & 0x0ff) &&
						isalnum((field_code >> 24) & 0x0ff))
						sprintf(buffer, "'%c%c%c%c' %ld bytes:\n", 
							(char)(field_code >> 24) & 0x0ff,
							(char)(field_code >> 16) & 0x0ff,
							(char)(field_code >> 8) & 0x0ff,
							(char)field_code & 0x0ff,
							numBytes);
					else
						sprintf(buffer, "0x%08lx %ld bytes:\n", field_code, numBytes);
					stream.Write(buffer, strlen(buffer));
					stream.Write("\n", 1);
					HexDumpToStream(data, numBytes, stream, tabs);
				}
				break;
			}
		}
	}
	stream.Write(tabs+2, tabCount-1);
	stream.Write("}\n", 2);
	return B_OK;
}

