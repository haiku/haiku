/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT license.
 */
#ifndef UTILITIES_H
#define UTILITIES_H

#include <String.h>


class BMessage;


BString trim_string(const char* string, size_t len);
	// Removes leading and trailing white space and replaces all sequences
	// of white space by a single space.


class Licenses {
public:
								Licenses(const char* license,...);
									// NULL terminated list
								Licenses(const BMessage& licenses);
									// all elements of field "License"
								~Licenses();

			int32				CountLicenses() const	{ return fCount; }
			const char*			LicenseAt(int32 index) const;

private:
			BString*			fLicenses;
			int32				fCount;
};


#endif	// UTILITIES_H
