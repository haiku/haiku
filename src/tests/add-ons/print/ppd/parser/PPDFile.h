/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _PPD_FILE_H
#define _PPD_FILE_H

#include <File.h>
#include <String.h>

class Position {
public:
	int x;
	int y;
	Position()             : x(0), y(0) {}
	Position(int x, int y) : x(x), y(y) {}
};

#define kReadBufferSize 1024

class FileBuffer {
	BFile* fFile;
	
	unsigned char fBuffer[kReadBufferSize];
	int           fIndex;
	int           fSize;
	
public:
	FileBuffer(BFile* file) : fFile(file), fIndex(0), fSize(0) {}
	int Read();
};

class PPDFile {
private:
	BString  fFileName;
	BFile    fFile;
	PPDFile* fPreviousFile; // single linked list of PPD files (stack)
	Position fCurrentPosition;
	int      fCurrentChar;
	FileBuffer fBuffer;
	
public:
	// Opens the file for reading. Use IsValid to check if the file could
	// be opened successfully.
	// PPDFile also maintance a single linked list. The parameter previousFile
	// can be used to store a reference to a previous file.
	PPDFile(const char* file, PPDFile* previousFile = NULL);
	// Closes the file.
	~PPDFile();
	
	// Returns the status of the constructor.
	status_t InitCheck();
	
	// Returns the current character or -1 if on EOF.
	int GetCurrentChar();
	// Reads the next character. Use GetChar to read the current 
	void NextChar();
	// Returns the position of the current character.
	Position GetPosition();
	// The previous file from the constructor.
	PPDFile* GetPreviousFile();
	// Returns the file name
	const char* GetFileName();
};

#endif
