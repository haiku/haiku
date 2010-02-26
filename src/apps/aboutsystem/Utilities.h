/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT license.
 */
#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdarg.h>

#include <InterfaceDefs.h>
#include <String.h>


class BMessage;


#define COPYRIGHT_STRING "Copyright " B_UTF8_COPYRIGHT " "


BString trim_string(const char* string, size_t len);
	// Removes leading and trailing white space and replaces all sequences
	// of white space by a single space.

void parse_named_url(const BString& namedURL, BString& name, BString& url);


class StringVector {
public:
								StringVector();
	explicit					StringVector(const char* string,...);
									// NULL terminated list
								StringVector(const BMessage& strings,
									const char* fieldName);
										// all elements of the given field
								StringVector(const StringVector& other);
								~StringVector();

			bool				IsEmpty() const			{ return fCount == 0; }
			int32				CountStrings() const	{ return fCount; }
			const char*			StringAt(int32 index) const;

			void				SetTo(const char* string,...);
			void				SetTo(const char* string, va_list strings);
			void				SetTo(const BMessage& strings,
									const char* fieldName,
									const char* prefix = NULL);
			void				Unset();


private:
			BString*			fStrings;
			int32				fCount;
};


class PackageCredit {
public:
								PackageCredit(const char* packageName);
								PackageCredit(
									const BMessage& packageDescription);
								PackageCredit(const PackageCredit& other);
								~PackageCredit();

			bool				IsValid() const;

			bool				IsBetterThan(const PackageCredit& other) const;

			PackageCredit&		SetCopyrights(const char* copyright,...);
			PackageCredit&		SetCopyright(const char* copyright);
			PackageCredit&		SetLicenses(const char* license,...);
			PackageCredit&		SetLicense(const char* license);
			PackageCredit&		SetSources(const char* source,...);
			PackageCredit&		SetSource(const char* source);
			PackageCredit&		SetURL(const char* url);

			const char*			PackageName() const;

			const StringVector&	Copyrights() const;
			int32				CountCopyrights() const;
			const char*			CopyrightAt(int32 index) const;

			const StringVector&	Licenses() const;
			int32				CountLicenses() const;
			const char*			LicenseAt(int32 index) const;

			const StringVector& Sources() const;
			int32				CountSources() const;
			const char*			SourceAt(int32 index) const;

			const char*			URL() const;

	static	bool				NameLessInsensitive(const PackageCredit* a,
									const PackageCredit* b);

private:
			int					_MaxCopyrightYear() const;

private:
			BString				fPackageName;
			StringVector		fCopyrights;
			StringVector		fLicenses;
			StringVector		fSources;
			BString				fURL;
};


#endif	// UTILITIES_H
