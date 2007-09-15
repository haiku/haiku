// $Id: QcIterator.hh,v 1.5 2000/12/06 05:32:56 pmoulder Exp $

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

#ifndef __QcIteratorH
#define __QcIteratorH

class QcIterator
{
public:
	virtual ~QcIterator() { }

	//-----------------------------------------------------------------------//
	// Iteration functions.                                                  //
	//-----------------------------------------------------------------------//
	//virtual void Advance() = 0;
	virtual bool AtEnd() const = 0;
		// Returns true iff the iterator is past the end of its row or column.
	virtual void Increment() = 0;
	virtual void Reset() = 0;
	//virtual void SetToBeginOf(int i) = 0;
};

#endif /* !__QcIteratorH */
