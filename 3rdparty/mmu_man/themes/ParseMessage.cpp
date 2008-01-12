#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <Debug.h>
#include <Font.h>
#include <String.h>
#include <Message.h>
#include <TypeConstants.h>

#include "DumpMessage.h"
#include "Utils.h"

#define MAX_TEXT_LINE_INPUT_SIZE 4096

class TextLineInputDataIO : public BDataIO {
public:
	TextLineInputDataIO(BDataIO *input);
	ssize_t Read(void *buffer, size_t size);
	char GetByte();
	ssize_t Write(const void *data, size_t len);
	status_t FillTextLine();
private:
	BDataIO *fIn;
	char fTextLine[MAX_TEXT_LINE_INPUT_SIZE];
	char *fCurrent; /* current char in fTextLine */
	int fLeft;
};

TextLineInputDataIO::TextLineInputDataIO(BDataIO *input)
{
	fIn = input;
	fCurrent = fTextLine;
	fLeft = 0;
}

char TextLineInputDataIO::GetByte()
{
	char buf[2];
	if (Read(buf, 1) == 1)
		return buf[0];
	return 0;
}
ssize_t TextLineInputDataIO::Read(void *buffer, size_t size)
{
	size = MIN(size, (size_t)fLeft);
	if (size <= 0)
		return 0;
	fLeft -= size;
	memcpy(buffer, fCurrent, size);
	fCurrent += size;
	return size;
}

ssize_t TextLineInputDataIO::Write(const void *, size_t)
{
	return EINVAL;
}

/*
 * \return 1 useful line, 0 blank lin or comment, <0 error
 */
status_t TextLineInputDataIO::FillTextLine()
{
	status_t err = ENOENT;
	
	fCurrent = fTextLine;
	while ((fCurrent < (fTextLine + MAX_TEXT_LINE_INPUT_SIZE)) 
					&& ((err = fIn->Read(fCurrent, 1)) > 0) 
					&& *fCurrent != '\n')
		fCurrent++;
	*fCurrent = '\0';
	fLeft = fCurrent - fTextLine;
	fCurrent = fTextLine;
//	printf("line: '%s'\n", fCurrent);
	
	if (err < B_OK)
		return err;
	if (err == 0)
		return -1; // EOF
	if (strlen(fCurrent) && strncmp(fCurrent, "//", 2))
		return 1;
	return 0;
}

status_t ParseHexDumpFromStream(const void **, size_t *, BDataIO &)
{
	return B_OK;
}

status_t Parse_Msg_bool(BMessage *msg, TextLineInputDataIO *st, char *name)
{
	char c;
	bool v = false;
	c = st->GetByte();
	if (c == '1' || c == 't')
		v = true;
	msg->AddBool(name, v);
	return B_OK;
}

status_t Parse_Msg_int32(BMessage *msg, TextLineInputDataIO *st, char *name)
{
	char c;
	int64 v = 0; // to forget about overflow
	bool opposite = false;
	while (isdigit(c = st->GetByte()) || c == '-') {
		if (c == '-')
			opposite = true;
		else
			v = v * 10 + (c - '0');
	}
	if (c == 'x' && v == 0) {
		// hex
		while (isalnum(c = st->GetByte())) {
			v = v * 16;
			if (isdigit(c))
				v += (c - '0');
			else if (isupper(c))
				v += (c - 'A' + 10);
			else
				v += (c - 'a' + 10);
		}
	}
	if (opposite)
		v = -v;
	msg->AddInt32(name, (int32)v);
	return B_OK;
}

status_t Parse_Msg_float(BMessage *msg, TextLineInputDataIO *st, char *name)
{
	char c;
	float v = 0;
	int32 div = 1;
	bool opposite = false;
	while (isdigit(c = st->GetByte()) || c == '-') {
		if (c == '-')
			opposite = true;
		else
			v = v * 10 + (c - '0');
	}
	if (c == '.')
		while (isdigit(c = st->GetByte())) {
			div *= 10;
			v += (float)(c - '0') / div;
		}
	if (opposite)
		v = -v;
	msg->AddFloat(name, v);
	return B_OK;
}

status_t Parse_Msg_string(BMessage *msg, TextLineInputDataIO *st, char *name)
{
	char c;
	BString v;
	bool esc = false;
	if (st->GetByte() != '"')
		return EINVAL;
	while ((c = st->GetByte()) || esc) {
		if (esc && !c) {
			v << '\n';
			esc = false;
			st->FillTextLine();
			continue;
		}
		if (esc) {
			esc = false;
		} else if (c == '\\') {
			esc = true;
			continue;
		} else if (c == '"')
			break;
		v << c;
	}
	msg->AddString(name, v.String());
	return B_OK;
}

status_t Parse_Msg_rgb_color(BMessage *msg, TextLineInputDataIO *st, char *name)
{
	char c;
	rgb_color v = {0, 0, 0, 0};
	bool has_alpha = false;
	while (isdigit(c = st->GetByte()))
		v.red = v.red * 10 + (c - '0');
	while (isdigit(c = st->GetByte()))
		v.green = v.green * 10 + (c - '0');
	while (isdigit(c = st->GetByte()))
		v.blue = v.blue * 10 + (c - '0');
	while (isdigit(c = st->GetByte())) {
		has_alpha = true;
		v.alpha = v.alpha * 10 + (c - '0');
	}
	if (!has_alpha)
		v.alpha = 255;
	AddRGBColor(*msg, name, v);
	return B_OK;
}

float Parse_Msg_read_float(TextLineInputDataIO *st)
{
	char c;
	int i;
	float f = 0.0F;
	float scale = 0.1F;
	bool isNegative = false;
	
	i=0;
	while ((c = st->GetByte()))
	{
		if (isdigit(c))
		{
			f = f * 10 + c - '0';
		}
		else if (c == '-')
		{
			isNegative = true;
		}
		else
		{
			break;
		}
	}
	while (isdigit(c = st->GetByte())) {
		f += (c - '0') * scale;
		scale /= 10;
	}
	
	if (isNegative)
		f *= -1.0;

	return f;
}

status_t Parse_Msg_BFont(BMessage *msg, TextLineInputDataIO *st, char *name)
{
	char c;
	int i;
	font_family ff;
	font_style fs;
	float size = 0.0f;
	BFont f;

	i=0;
	while ((c = st->GetByte()) != '/')
		ff[i++] = c;
	ff[i] = '\0';
	i=0;
	while ((c = st->GetByte()) != '/')
		fs[i++] = c;
	fs[i] = '\0';
	
	size = Parse_Msg_read_float(st);
	
//printf("fam '%s', style '%s'\n", ff, fs);
	f.SetFamilyAndStyle(ff, fs);
	f.SetSize(size);
	//f.PrintToStream();
	AddFont(*msg, name, &f);
	return B_OK;
}

status_t Parse_Msg_BPoint(BMessage *msg, TextLineInputDataIO *st, char *name)
{
	char c;
	int i;
	BPoint p;
	
	i=0;
	p.x = Parse_Msg_read_float(st);
	if ((c = st->GetByte()) != ' ')
		return B_ERROR;
	p.y = Parse_Msg_read_float(st);
	
//printf("BPoint(%f, %f)\n", p.x, p.y);
	msg->AddPoint(name, p);
	return B_OK;
}

status_t Parse_Msg_HexDump(BMessage *msg, TextLineInputDataIO *st, char *name, int32 field_code)
{
	(void)msg;
	(void)st;
	(void)name;
	(void)field_code;
	return B_OK;
}

status_t Parse_Msg_BMessage(BMessage *msg, TextLineInputDataIO *st)
{
	char field_name[B_FIELD_NAME_LENGTH];
	char field_code_name[B_FIELD_NAME_LENGTH];
	type_code field_code;
	int32 index; // unused !
	int i = 0;
	char c;
	status_t err;
	
	c = st->GetByte();
	if (c == '\'')  {
		msg->what = ((st->GetByte() << 24) & 0xff000000) |	
					((st->GetByte() << 16) & 0x0ff0000) |
					((st->GetByte() << 8) & 0x0ff00) |
					(st->GetByte() & 0x0ff);
		if (st->GetByte() != '\'')
			return EINVAL;
		if (st->GetByte() != ')')
			return EINVAL;
	} else if (c == '0') {
		if (st->GetByte() != 'x')
			return EINVAL;
		while (isalnum(c = st->GetByte())) {
			//printf("[%c]", c);
			if (c >= 'a' && c <= 'f')
				msg->what = (msg->what << 4) | (c - 'a' + 10);
			else if (c >= 'A' && c <= 'F')
				msg->what = (msg->what << 4) | (c - 'A' + 10);
			else if (c >= '0' && c <= '9')
				msg->what = (msg->what << 4) | (c - '0');
			else return EINVAL;
		}
	} else
		return EINVAL;
	if (st->GetByte() != ' ')
		return EINVAL;
	if (st->GetByte() != '{')
		return EINVAL;
	//printf("what=0x%08lx\n", msg->what);
	
	while ((err = st->FillTextLine()) >= 0) {
		if (err == 0)
			continue;
		

		do
			c = field_name[0] = st->GetByte();
		while (c == '\t' || c == ' ');
	
		for (i = 1; (c = st->GetByte()) && (c != '=') && (c != '['); i++)
			field_name[i] = c;
		field_name[i] = '\0';
		if (i>1 && field_name[i-1] == ' ')
			field_name[i-1] = '\0';

		if (!strcmp(field_name, "}"))
			return B_OK;

		if (c == '[') {
			for (index = 0; (c = st->GetByte()) && isdigit(c); index = index * 10 + c - '0');
			for (; (c = st->GetByte()) && (c != '='); );
			//PRINT(("index ignored (%ld) for item '%s'\n", index, field_name));
		}

		i = 0;
		do
			c = field_code_name[0] = st->GetByte();
		while (c && (c == '\t' || c == ' '));
	
//		while ((field_code_name[++i] = st->GetByte()) != '(');
		for (i = 1; (c = st->GetByte()) && (c != '(') && (c != ' '); i++)
			field_code_name[i] = c;
		field_code_name[i] = '\0';
		
		if (!strncmp(field_name, "//", 2))
			continue;
		
//		printf("name:'%s' code'%s'\n", field_name, field_code_name);

		if (!strcmp(field_code_name, "BMessage")) {
			BMessage *m;
			m = new BMessage;
			err = Parse_Msg_BMessage(m, st);
			if (err < B_OK) {
				delete m;
				return err;
			}
			msg->AddMessage(field_name, m);
		} else if (!strcmp(field_code_name, "bool")) {
			err = Parse_Msg_bool(msg, st, field_name);
			if (err < B_OK)
				return err;
		} else if (!strcmp(field_code_name, "int32")) {
			err = Parse_Msg_int32(msg, st, field_name);
			if (err < B_OK)
				return err;
		} else if (!strcmp(field_code_name, "float")) {
			err = Parse_Msg_float(msg, st, field_name);
			if (err < B_OK)
				return err;
		} else if (!strcmp(field_code_name, "string")) {
			err = Parse_Msg_string(msg, st, field_name);
			if (err < B_OK)
				return err;
		} else if (!strcmp(field_code_name, "rgb_color")) {
			err = Parse_Msg_rgb_color(msg, st, field_name);
			if (err < B_OK)
				return err;
		} else if (!strcmp(field_code_name, "BPoint")) {
			err = Parse_Msg_BPoint(msg, st, field_name);
			if (err < B_OK)
				return err;
		} else if (!strcmp(field_code_name, "BFont")) {
			err = Parse_Msg_BFont(msg, st, field_name);
			if (err < B_OK)
				return err;
		} else if (!strncmp(field_code_name, "'", 1)) {
			field_code = ((st->GetByte() << 24) & 0xff000000) |	
						((st->GetByte() << 16) & 0x0ff0000) |
						((st->GetByte() << 8) & 0x0ff00) |
						(st->GetByte() & 0x0ff);
			err = Parse_Msg_HexDump(msg, st, field_name, field_code);
			if (err < B_OK)
				return err;
		} else
			return EINVAL;
		
	}
	return B_OK;
}

status_t ParseMessageFromStream(BMessage **message, BDataIO &stream)
{
//	char field_name[B_FIELD_NAME_LENGTH];
	char field_code_name[B_FIELD_NAME_LENGTH];
//	type_code field_code;
//	int32 field_count;
	int i = 0;
	char c;
	status_t err;

	if (message == NULL)
		return EINVAL;
	
	BMessage *msg = *message = new BMessage;
	TextLineInputDataIO *st = new TextLineInputDataIO(&stream);
	while ((err = st->FillTextLine()) == 0);
	if (err < 0)
		return EINVAL;


	i = 0;
	do
		c = field_code_name[0] = st->GetByte();
	while (c && (c == '\t' || c == ' '));
	
//	while ((field_code_name[++i] = st->GetByte()) != '(');
	for (i = 1; (c = st->GetByte()) && (c != '('); i++)
		field_code_name[i] = c;
	field_code_name[i] = '\0';
	

	//printf("code'%s'\n", field_code_name);
	if (strcmp(field_code_name, "BMessage")) {
		PRINT(("bad code name (%s)\n", field_code_name));
		return EINVAL;
	}
	err = Parse_Msg_BMessage(msg, st);
	if (err < B_OK) {
		delete msg;
		*message = NULL;
		return B_ERROR;
	}

	return B_OK;
}

