// $Id: QcException.hh,v 1.3 2000/11/21 04:26:12 pmoulder Exp $

//============================================================================//
// Written by Alan Finlay and Sitt Sen Chok                                   //
//----------------------------------------------------------------------------//
// The QOCA implementation is free software, but it is Copyright (C)          //
// 1994-1999 Monash University.  It is distributed under the terms of the GNU //
// General Public License.  See the file COPYING for copying permission.      //
//                                                                            //
// The QOCA toolkit and runtime are distributed under the terms of the GNU    //
// Library General Public License.  See the file COPYING.LIB for copying      //
// permissions for those files.                                               //
//                                                                            //
// If those licencing arrangements are not satisfactory, please contact us!   //
// We are willing to offer alternative arrangements, if the need should arise.//
//                                                                            //
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED OR  //
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.                                     //
//                                                                            //
// Permission is hereby granted to use or copy this program for any purpose,  //
// provided the above notices are retained on all copies.  Permission to      //
// modify the code and to distribute modified code is granted, provided the   //
// above notices are retained, and a notice that the code was modified is     //
// included with the above copyright notice.                                  //
//============================================================================//

#ifndef __QcExceptionH
#define __QcExceptionH

#include <stdlib.h>
#include "qoca/QcDefines.hh"

#ifndef qcNoStream
#include <iostream>
#endif

//#ifdef DEBUG
//void QcBreak(int);	// For trapping warnings when using debugger
//#else
#define QcBreak(x)
//#endif

#ifdef qcUseExceptions
class QcError
{
public:
	QcError(const char *m)
		: fMessage(m), fFile(0), fLine(-1), fIndex(-1), fRange(-1)
		{ QcBreak(1); }

	QcError(const char *f, int ln)
		: fMessage("Assertion failed"), fFile(f), fLine(ln), fIndex(-1),
		fRange(-1)
		{ QcBreak(2); }

	QcError(const char *f, int ln, const char *m)
		: fMessage(m), fFile(f), fLine(ln), fIndex(-1), fRange(-1)
		{ QcBreak(3); }

	QcError(const char *m, int i, int r)
		: fMessage(m), fFile(0), fLine(-1), fIndex(i), fRange(r)
		{ QcBreak(4); }

	QcError(const char *f, int ln, const char *m, int i, int r)
		: fMessage(m), fFile(f), fLine(ln), fIndex(i), fRange(r)
		{ QcBreak(5); }

private:
	const char *fMessage;
	const char *fFile;
	const int fLine;
	const int fIndex;
	const int fRange;
};
#endif

#ifdef qcUseExceptions
class QcWarning : public QcError
{
public:
	QcWarning(const char *m)
    		: QcError(m)
		{ }

	QcWarning(const char *file, int line, const char *m)
		: QcError(file, line, m)
		{ }

	QcWarning(const char *m, int i, int r)
		: QcError(m, i, r)
		{ }

	QcWarning(const char *file, int line, int i, int r)
		: QcError(file, line, 0, i, r)
		{ }

	QcWarning(const char *file, int line, const char *m, int i, int r)
		: QcError(file, line, m, i, r)
		{ }
};
#else
#define throw
class QcWarning
{
public:
	QcWarning(const char *m);
	QcWarning(const char *file, int line, const char *m);
	QcWarning(const char *m, int i, int r);
	QcWarning(const char *file, int line, int i, int r);
	QcWarning(const char *file, int line, const char *m, int i, int r);
};

inline QcWarning::QcWarning(const char *m)
{
	cout << "Warning: " << m << endl;
	QcBreak(1);
}

inline Warning::Warning(const char *file, int line, const char *m)
{
	cout << "Warning: " << m;
	cout << ", at line " << line << " of " << file << endl;
	QcBreak(2);
}

inline Warning::Warning(const char *m, int i, int r)
{
	cout << "Warning: " << m
    		<< ", index " << i << ", range (0," << r << ")" << endl;
	QcBreak(3);
}

inline Warning::Warning(const char *file, int line, int i, int r)
{
	cout << "Warning: out of bounds, index " << i << ", range (0,";
	cout << r << ") at line " << line << " of " << file << endl;
	QcBreak(4);
}

inline Warning::Warning(const char *file, int line, const char *m, int i, int r)
{
	cout << "Warning: " << m << ", index " << i << ", range (0,";
	cout << r << ") at line " << line << " of " << file << endl;
	QcBreak(5);
}
#endif

#ifdef qcUseExceptions
class QcFatal : public QcError
{
public:
	QcFatal(const char *m)
		: QcError(m)
		{ }

	QcFatal(const char *file, int line, const char *m)
		: QcError(file, line, m)
		{ }

	QcFatal(const char *m, int i, int r)
		: QcError(m, i, r)
		{ }

	QcFatal(const char *file, int line, int i, int r)
		: QcError(file, line, 0, i, r)
		{ }

	QcFatal(const char *file, int line, const char *m, int i, int r)
		: QcError(file, line, m, i, r)
		{ }
};
#else
#define throw
class QcFatal
{
public:
	QcFatal(const char *m);
	QcFatal(const char *file, int line, const char *m);
	QcFatal(const char *m, int i, int r);
	QcFatal(const char *file, int line, int i, int r);
	QcFatal(const char *file, int line, const char *m, int i, int r);
};

inline QcFatal::QcFatal(const char *m)
{
	cout << "QcFatal error: " << m << endl;
	QcBreak(6);
	exit(1);
}

inline QcFatal::QcFatal(const char *file, int line, const char *m)
{
	cout << "QcFatal error: " << m;
	cout << ", at line " << line << " of " << file << endl;
	QcBreak(7);
	exit(1);
}

inline QcFatal::QcFatal(const char *m, int i, int r)
{
	cout << "QcFatal error: " << m
		<< ", index " << i << ", range (0," << r << ")" << endl;
	QcBreak(8);
	exit(1);
}

inline QcFatal::QcFatal(const char *file, int line, int i, int r)
{
	cout << "QcFatal error: out of bounds, index " << i << ", range (0,";
	cout << r << ") at line " << line << " of " << file << endl;
	QcBreak(9);
	exit(1);
}

inline QcFatal::QcFatal(const char *file, int line, const char *m, int i, int r)
{
	cout << "QcFatal error: " << m << ", index " << i << ", range (0,";
	cout << r << ") at line " << line << " of " << file << endl;
	QcBreak(10);
	exit(1);
}
#endif

#endif
