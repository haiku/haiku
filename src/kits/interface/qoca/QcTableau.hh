// $Id: QcTableau.hh,v 1.11 2001/01/30 01:32:08 pmoulder Exp $

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
//----------------------------------------------------------------------------//

#ifndef __QcTableauH
#define __QcTableauH

#include "qoca/QcDefines.hh"
#include "qoca/QcBiMapNotifier.hh"

// Forward declaration.
class QcRowAdaptor;

/** Constraint tableau.  Tableau rows correspond to contraints.  Tableau columns
    correspond to variables.  RHS is a separately stored column.  The tableau is
    at a level of abstraction that does not access <tt>QcFloat</tt>s, hence the
    constraints represented in the tableau cannot be evaluated at this level.
    The client maintains the mapping from variable index to <tt>QcFloat</tt> and
    can evaluate a constraint using this information and a
    <tt>QcSparseRowIterator</tt>.
**/

class QcTableau
{
public:
	enum {fInvalidVariableIndex = -1, fInvalidConstraintIndex = -1};

protected:
	QcBiMapNotifier &fNotifier;	// Must be initialised by constructors
	QcConstraintBiMap &fOCBiMap;
	QcVariableBiMap &fVBiMap;

public:
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcTableau(QcBiMapNotifier &n);

	virtual ~QcTableau()
		{ }

#ifndef NDEBUG
  void assertInvar() const { }

  void
  assertDeepInvar() const
  {
    fNotifier.vAssertDeepInvar();
    fVBiMap.vAssertDeepInvar();
    fOCBiMap.vAssertDeepInvar();
  }

  virtual void
  vAssertDeepInvar() const
  {
    assertDeepInvar();
  }
#endif

	//-----------------------------------------------------------------------//
	// Data structure access functions.                                      //
	//-----------------------------------------------------------------------//
	QcBiMapNotifier &getNotifier()
		// GetNotifier is not particularly useful since the notifier
		// base class interface is not intended for clients to use.
		// The client oriented interface is in the derived class only.
  		{ return fNotifier; }

	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//
	// The GetRows() and GetColumns() bounds are useful in determining the   //
	// size of the domain should a map from constraint index or variable     //
	// index be needed.  It should not be assumed that all the values in the //
	// stated range are utilised.  For example when rows are removed from the//
	// derived class QcLinEqTableau the constraint index for that row becomes//
	// unused.  GetValue() can be called for unused indices and will return  //
	// zero.                                                                 //
	//-----------------------------------------------------------------------//
	virtual int GetColumns() const = 0;
	virtual unsigned getNColumns() const = 0;
	virtual int GetBasicVar(unsigned ci) const = 0;
		// Indicates the basic variable for solved form constraint ci.
	virtual numT GetRHS(unsigned ci) const = 0;
	virtual int GetRows() const = 0;
	virtual numT GetValue(unsigned ci, unsigned vi) const = 0;
	virtual bool IsBasic(unsigned vi) const = 0;
	virtual int IsBasicIn(unsigned vi) const = 0;
  	virtual bool IsRedundant(unsigned ci) const = 0;
		// Indicates if the original constraint ci is redundant.

	virtual bool IsFree(unsigned vi) const;
		// Indicates if a particular variable is in use (i.e. has a
		// non-zero coefficient in any solved form (or original) constraint.

	//-----------------------------------------------------------------------//
	// Manipulation functions.                                               //
	//-----------------------------------------------------------------------//
	virtual int AddEq(QcRowAdaptor &varCoeffs, numT rhs) = 0;
		// Returns the solved form constraint index of the new constraint.
	virtual void ChangeRHS(unsigned ci, numT rhs) = 0;
		// Change the RHS of the constraint ci.


	/** Eliminate the basic variables from new original constraint ci
	    creating a new solved form constraint and returning this new index.

	    <p>Call this method after addEq.

	    <p>If the new constraint is consistant, Eliminate moves the row
	    condition from invalid to normalised or redundant.

	    <p>The new solved form constraint still needs a Pivot to put it into
	    solved form.  If the new solved form is inconsistant, the original
	    constraint is removed and InvalidCIndex is returned.  If Eliminate
	    returns a valid index, the new solved form constraint may be
	    redundant.  Check with IsRedundant before attempting a Pivot.
	
	    <p>Eliminate updates the current PivotHints.

	    @precondition <tt>ci &lt; getNRows()</tt>
	    @precondition <tt>!fCoreTableau-&gt;GetARowDeleted(ci)</tt>
	**/
	virtual int Eliminate(unsigned ci) = 0;

	/** Allocate a new variable column. */
	virtual unsigned NewVariable() = 0;

	virtual bool Pivot(unsigned ci, unsigned vi) = 0;
		// Pivot returns true if successful, some derived classes may
		// indicate failure, in which case the tableau is unchanged.
	virtual void Restart() = 0; 	// Erase everything ready to start afresh.

	virtual int RemoveEq(unsigned ci);
		// Removes an equation from the tableau given its original constraint
		// index.  Returns the corresponding solved form constraint index.
		// After an equation is removed the CIndex values for the remaining
		// equations are unchanged.  CIndex values are not shifted up to
		// remove the "hole" left by the removed equation.

	virtual bool RemoveVarix(unsigned vi);
		// Remove the variable vi from the tableau.

	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
	virtual void Print(ostream &os) const;
	virtual void TransitiveClosure(vector<unsigned int> &vars);
};

inline QcTableau::QcTableau(QcBiMapNotifier &n)
		: fNotifier(n), fOCBiMap(n.GetOCMap()), fVBiMap(n.GetVMap())
{
}


/* Note: the reason that IsFree, RemoveEq, RemoveVarix,
   TransitiveClosure are here is that QcLinEqSystem has a var of type
   QcLinEqTableau&, and QcDelLinEqSystem puts a QcDelLinEqTableau in
   that var.  Maybe we should separate QcLinEqSystem into the bits
   that are and aren't shared with QcDelLinEqSystem, with the fTableau
   var not being shared.  The common base can define some abstract
   methods to access the fTableau var.  Advantage of splitting: type
   safety and possible inlining/specialization advantages because the
   compiler knows what type fTableau is. */
inline bool QcTableau::IsFree(unsigned vi) const
{
	UNUSED(vi);
	throw QcWarning("Error: Should not call QcTableau::IsFree");
  	return false;
}

inline void QcTableau::Print(ostream &os) const
{
	os << "Begin[Tableau]" << endl;
	fNotifier.Print(os);
	os << "End[Tableau]" << endl;
}

inline int QcTableau::RemoveEq(unsigned ci)
{
	UNUSED(ci);
	throw QcWarning("Error: Should not call QcTableau::RemoveEq");
  	return fInvalidConstraintIndex;
}

inline bool QcTableau::RemoveVarix(unsigned vi)
{
	UNUSED(vi);
	throw QcWarning("Error: Should not call QcTableau::RemoveVar");
  	return false;
}

inline void QcTableau::TransitiveClosure(vector<unsigned int> &vars)
{
	UNUSED(vars);
	throw QcWarning("Error: Should not call QcTableau::TransitiveClosure");
}

#endif
