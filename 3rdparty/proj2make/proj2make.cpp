/*
 * Copyright 2012 Aleksas Pantechovskis, <alexp.frl@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <exception>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>

#include <ByteOrder.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>
#include <TypeConstants.h>


using namespace std;


const char* kUsageMessage = \
	"proj2make usage:\n"
	"# proj2make <projPath> [makePath]\n"
	"# if makePath parameter doesn't specified makefile will be created in\n"
	"#	the same directory as .proj file\n"
	"# example: proj2make /boot/home/myprog/myprog.proj\n";

fstream gProjFile;
uint32 gProjLength;
uint8* gProjData;

fstream gMakeFile;

fstream gTemplateFile;

string gSPthString;
string gPPthString;
string gFil1String;
string gLinkString;
string gPLnkString;

const char* gAppTypes[] = {
	"APP",
	"SHARED",
	"STAITC",
	"DRIVER"
};

uint8 gAppType;
string gAppName;

struct hdr
{
			uint32	Id() { return
						static_cast<uint32>(B_BENDIAN_TO_HOST_INT32(fId)); }
			uint32	Size() { return
						static_cast<uint32>(B_BENDIAN_TO_HOST_INT32(fSize)); }
			const char* Data() { return (char*)(this + 1); }
private:
			uint32 fId;
			uint32 fSize;
};


class Error : public std::exception
{
			BString		fWhat;
public:
						Error(const char* what, ...);
	virtual				~Error() throw() {}
	virtual const char*	what() const throw() { return fWhat.String(); }
};


Error::Error(const char* what, ...)
{
	const int size = 1024;
	va_list args;
	va_start(args, what);
	vsnprintf(fWhat.LockBuffer(size), size, what, args);
	fWhat.UnlockBuffer();
	va_end(args);
}


void
CheckFiles(const char* projPath, const char* makePath)
{
	gProjFile.open(projPath, fstream::in | fstream::binary);
	if (!gProjFile.is_open())
		throw Error("%s not found", projPath);

	gProjFile.seekg(0, ios::end);
	uint32 projFileLength = gProjFile.tellg();
	gProjFile.seekg(0, ios::beg);

	char* name = new char[5];
	gProjFile.read(name, 4);

	uint32 length;
	gProjFile.read((char*)&length, 4);
	name[4] = '\0';
	length = static_cast<uint32>(B_BENDIAN_TO_HOST_INT32(length));
	gProjLength = length + 8;

	if (strcmp(name, "MIDE") != 0 || gProjLength > projFileLength)
		throw Error("File corrupted or it is not BeIDE *.proj file");

	gMakeFile.open(makePath, fstream::in);
	if (gMakeFile.is_open())
		throw Error("%s already exists", makePath);

	gMakeFile.open(makePath, fstream::out);
	if (!gMakeFile.is_open())
		throw Error("Can not create makefile");

	BPath templateFileName;
	// not supporter yet in my haiku rev
	// find_directory(B_SYSTEM_DEVELOP_DIR, &templateFileName);
	// templateFileName.Append("etc/makefile");
	templateFileName.SetTo("/boot/develop/etc/makefile");

	gTemplateFile.open(templateFileName.Path(), fstream::in);
	if (!gTemplateFile.is_open())
		throw Error("Can not open template %s", templateFileName.Path());
}


void
ParseGenB(hdr* data)
{
	hdr* child = (hdr*)data->Data();
	char* name = (char*)(child + 1);
	int len = strlen(name) + 1;

	uint32 u = child->Id();
	char* c = (char*)&u;
	printf("\t%c%c%c%c:%d:%s\n", c[3], c[2], c[1], c[0], child->Size(), name);

	if (strncmp(name, "ProjectPrefsx86", len - 1) == 0) {
		const char* type = child->Data() + len + 8;
		if (*type <= 3)
			gAppType = *type;
		type++;
		type += 64; // skip the mime type name
		gAppName = type;
	}
}


class _l {
	static string _s;
public:
	_l() { _s += " " ; }
	~_l() { _s.resize(_s.size() - 1); }

	char* str() { _s.c_str(); }
};

string _l::_s;

void
Parse(hdr* current, hdr* parent)
{
	_l l;

	uint32 u = current->Id();
	char* c = (char*)&u;
	printf("%#06x:%s%c%c%c%c:%d\n",
		(uint8*)current - gProjData, l.str(),
		c[3], c[2], c[1], c[0], current->Size());

	bool useGrandParent = false;
	size_t off = 0;
	switch(current->Id()) {
		case 'Fil1':
		case 'Link':
		case 'PLnk':
			off = 24;
			break;
		case 'MIDE':
		case 'DPrf':
		case 'GPrf':
			break;
		case 'MSFl':
			off = 8;
			useGrandParent = true;
			break;
		case 'SPth':
			gSPthString += " \\\n\t";
			gSPthString += &current->Data()[5];
			return;
		case 'PPth':
			gPPthString += " \\\n\t";
			gPPthString += &current->Data()[5];
			return;
		case 'Name':
			if (parent->Id() == 'Fil1') {
				gFil1String += " \\\n\t";
				gFil1String += &current->Data()[4];
			} else if (parent->Id() == 'Link') {
				gLinkString += " \\\n\t";
				gLinkString += &current->Data()[4];
			} else if (parent->Id() == 'PLnk') {
				gPLnkString += " \\\n\t";
				gPLnkString += &current->Data()[4];
			}
			return;
		case 'GenB':
			ParseGenB(current);
			return;
		default:
			return;
	}

	hdr* child = (hdr*)(current->Data() + off);
	while (off < current->Size()) {
		Parse(child, useGrandParent ? parent : current);
		off += child->Size() + sizeof(hdr);
		child = (hdr*)(child->Data() + child->Size());
	}
}


void
ReadProj()
{
	gProjFile.seekg(0, ios::beg);
	gProjData = new uint8[gProjLength];
	gProjFile.read((char*)gProjData, gProjLength);
	gProjFile.close();

	Parse((hdr*)gProjData, NULL);
}


void
Proj2Make()
{
	gFil1String = " ";
	gLinkString = " ";
	gPLnkString = " ";
	gSPthString = " ";
	gPPthString = " ";

	ReadProj();
	string str;
	while (gTemplateFile.good()) {
		getline(gTemplateFile, str);

		if (str.find("SRCS") == 0)
			str = str + gFil1String;
		else if (str.find("LIBS") == 0)
			str = str + gLinkString;
		else if (str.find("SYSTEM_INCLUDE_PATHS") == 0)
			str = str + gSPthString;
		else if (str.find("LOCAL_INCLUDE_PATHS") == 0)
			str = str + gPPthString;
		else if (str.find("TYPE") == 0)
			str = str + gAppTypes[gAppType];
		else if (str.find("NAME") == 0)
			str = str + gAppName;
		else if (str.find("RSRCS") == 0)
			str = str + gPLnkString;

		gMakeFile << str << endl;
	}

	gMakeFile.close();
	gTemplateFile.close();
}


int
main(int argc, char** argv)
{
	try {
		if (argc <= 1 || (argc > 1 && strcmp(argv[1], "--help") == 0))
			throw Error("");

		BString projPath = argv[1];

		BString makePath;
		// if makefile path specified
		if (argc > 2)
			makePath = argv[2];
		// default makefile path
		else {
			BPath path(argv[1]);
			path.GetParent(&path);
			path.Append("makefile");
			makePath = path.Path();
		}

		CheckFiles(projPath.String(), makePath.String());

		Proj2Make();

	} catch(exception& exc) {
		cerr << argv[0] << " : " << exc.what() << endl;
		cerr << kUsageMessage;
		return B_ERROR;
	}

	return B_OK;
}

