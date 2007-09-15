// $Id: QcCassSolver.hh,v 1.8 2001/01/04 05:20:11 pmoulder Exp $

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
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY	EXPRESSED OR  //
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.                                     //
//                                                                            //
// Permission is hereby granted to use or copy this program for any purpose,  //
// provided the above notices are retained on all copies.  Permission to      //
// modify the code and to distribute modified code is granted, provided the   //
// above notices are retained, and a notice that the code was modified is     //
// included with the above copyright notice.                                  //
//============================================================================//

#ifndef __QcCassSolverH
#define __QcCassSolverH

#include <map>
#include "qoca/QcDelLinInEqSystem.hh"
#include "qoca/QcCassConstraint.hh"

using namespace std;

class QcCassSolver : public QcDelLinInEqSystem
{
protected:
  	typedef map<QcFloat, QcCassConstraint *> TStayMap;
  	typedef vector<QcCassConstraint *> TEditMap;
  	typedef vector<QcCassConstraint *> TStaySeqMap;

  	TStayMap fStayMap;
  	TEditMap fEditMap;
	TStaySeqMap fStaySeqMap;
  		// Corresponding data structure to fStayMap.  This is used to speed
		// things up when there is a need to iterate through all the stay
		// constraints.  Should not use fStayMap for iteration is not very
		// efficient.

public:
	//-----------------------------------------------------------------------//
	// Constructors.                                                         //
	//-----------------------------------------------------------------------//
	QcCassSolver();
	QcCassSolver(unsigned hintNumConstraints, unsigned hintNumVariables);
	virtual ~QcCassSolver();

	//-----------------------------------------------------------------------//
	// Variable management methods                                           //
	//-----------------------------------------------------------------------//
	virtual void AddVar(QcFloat &v);
   	virtual bool RemoveVar(QcFloat &v);

	//-----------------------------------------------------------------------//
	// Constraint management methods                                         //
	//-----------------------------------------------------------------------//
   	virtual bool AddConstraint(QcConstraint &c);
   	virtual bool AddConstraint(QcConstraint &c, QcFloat &hint);
	virtual void BeginAddConstraint();
	virtual bool EndAddConstraint();
	virtual bool ChangeConstraint(QcConstraint &oldc, numT rhs);
	virtual void InitObjective();
	virtual bool RemoveConstraint(QcConstraint &c);
	virtual bool Reset();

	//-----------------------------------------------------------------------//
	// High Level Edit Variable Interface for use by solver clients.         //
	//-----------------------------------------------------------------------//
	virtual void AddEditVar(QcFloat &v);
	virtual void EndEdit();
	virtual void BeginEdit();

	//-----------------------------------------------------------------------//
	// Constraint Solving methods                                            //
	//-----------------------------------------------------------------------//
   	virtual void Solve();
   	virtual void Resolve();

	void LoadDesValCache();
	void RawSolve();

	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
  	virtual void Restart();
  	virtual void Print(ostream &os) const;
};

inline QcCassSolver::QcCassSolver()
		: QcDelLinInEqSystem()
{
}

inline QcCassSolver::QcCassSolver(unsigned hintNumConstraints, unsigned hintNumVariables)
	: QcDelLinInEqSystem(hintNumConstraints, hintNumVariables)
{
}

inline bool QcCassSolver::ChangeConstraint(QcConstraint &oldc, numT rhs)
{
	bool success = QcDelLinInEqSystem::ChangeConstraint(oldc, rhs);
#if qcCheckPost
	if (success)
		changeCheckedConstraint( oldc.pointer(), rhs);
#endif
	return success;
}

inline bool QcCassSolver::RemoveConstraint(QcConstraint &c)
{
	bool success = QcDelLinInEqSystem::RemoveConstraint(c);
	if (success) {
#if qcCheckPost
		removeCheckedConstraint( c.pointer());
#endif
		if (fAutoSolve)
			Solve();
	}
	return success;
}

#endif /* !__QcCassSolverH */
