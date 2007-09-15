// $Id: QcLinEqSystem.hh,v 1.10 2001/01/30 01:32:08 pmoulder Exp $

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

#ifndef __QcLinEqSystemH
#define __QcLinEqSystemH

#include <vector>
#include "qoca/QcSolver.hh"
#include "qoca/QcLinEqTableau.hh"
#include "qoca/QcConstraint.hh"
#include "qoca/QcBiMapNotifier.hh"
#include "qoca/QcSparseCoeff.hh"
#include <qoca/QcVariableBiMap.hh>

class QcLinEqSystem : public QcSolver
{
public:
	typedef QcVariableBiMap::const_identifier_iterator const_var_iterator;

protected:
	vector<unsigned int> fDepVars;
  	vector<unsigned int> fParsOfInterest;
	vector<unsigned int> fVarsByIndex;	// Indices of variables of interest.
	vector<numT> fDepVarBaseVals;	// Used for Resolve
	vector<vector<QcSparseCoeff> > fDepVarCoeffs;
		// fDepVarCoeffs is a partial map from dependant variable index
		// to a vector (used as a list) of sparse coefficients.

  	QcVariableBiMap &fVBiMap;
	QcBiMapNotifier fNotifier;
  	QcConstraintBiMap &fOCBiMap;
	QcLinEqTableau &fTableau;

	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
public:
	QcLinEqSystem();
	QcLinEqSystem(unsigned hintRows, unsigned hintCols);
	QcLinEqSystem(QcLinEqTableau &tab);
	virtual ~QcLinEqSystem();

#if qcCheckInternalInvar
  void assertInvar() const { }
  void assertDeepInvar() const;
  virtual void vAssertDeepInvar() const;
#endif

	//-----------------------------------------------------------------------//
	// Data Structure query functions                                        //
	//-----------------------------------------------------------------------//
	QcBiMapNotifier GetNotifier()
		{ return fNotifier; }

  	QcConstraintBiMap &GetOCBiMap()
		{ return fOCBiMap; }

  	QcVariableBiMap &GetVBiMap()
		{ return fVBiMap; }

	QcLinEqTableau &GetTableau()
		{ return fTableau; }

	//-----------------------------------------------------------------------//
	// Variable management methods (see QcSolver.java for descriptions)      //
	//-----------------------------------------------------------------------//
	virtual void AddVar(QcFloat &v);
		// N.B. AddVar does nothing if the variable is already registered.
  	virtual void SuggestValue(QcFloat &v, numT desval);
		// Sets the desired value of v to desval.
	virtual void RestSolver();

	//-----------------------------------------------------------------------//
	// Enquiry functions for variables                                       //
	//-----------------------------------------------------------------------//
	virtual bool IsRegistered(QcFloat const &v) const
		{ return (fVBiMap.IdentifierPresent(v)); }

	virtual bool IsBasic(QcFloat const &v) const
		{ return fTableau.IsBasic(fVBiMap.Index(v)); }

	//-----------------------------------------------------------------------//
	// High Level Edit Variable Interface for use by solver clients.         //
	// The methods addEditVar and endEdit are more efficient than the ones   //
	// inherited from Solver since variable weights have no effect on        //
	// QcLinEqSystem::Resolve.                                               //
  	//-----------------------------------------------------------------------//
	virtual void BeginEdit();
		// Implementation note.
		// It does not appear to be necessary to call solve before
		// BeginEdit, so don't depend on this in the first loop.
	virtual void EndEdit();
		// Specialised version of EndEdit since LinEqSystem does not adjust
		// weights.

	//-----------------------------------------------------------------------//
	// Constraint management methods                                         //
	//-----------------------------------------------------------------------//
	virtual bool AddConstraint(QcConstraint &c);
		// Extracts the various components and calls Insert, as well as
		// updating the bimapnotifier for the tableau if successful. The 
		// return value indicates success or failure. If AddConstraint fails
		// the state of the tableau is unchanged.
	virtual bool AddConstraint(QcConstraint &c, QcFloat &hint);
	virtual bool ChangeConstraint(QcConstraint &c, numT rhs);
	virtual bool Reset();
		// While this routine is in progress the tableau is slowly rebuilt
		// from row 0 forwards retaining the original constraint matrix fA.
		// The implementation used here only relies on the rest of the tableau
		// methods being able to cope with multiple undetermined constraints.
		// Reset starts by calling Restore to establish a state equivalent
		// to the original constraints having been all entered using AddEq
		// and nothing else apart from the deleted entries being left alone.

	//-----------------------------------------------------------------------//
	// Constraint Solving methods                                            //
	//-----------------------------------------------------------------------//
	virtual void Solve();
		// Using the tableau's desired values cache, calculate the value
		// for each variable depending upon whether it is basic or parametric.
	virtual void Resolve();

protected:
	//-----------------------------------------------------------------------//
	// Read access to the Tableau and BimapNotifier are a low level aspect of//
	// the public interface.                                                 //
	//-----------------------------------------------------------------------//
	virtual bool ApplyHints(int cs);
		// ApplyHints calls Pivot according to the PivotHints and should
		// only be called after a successful Eliminate call (i.e. the
		// new constraint is consistent and independent).  If all the
		// PivotHints fail then ApplyHints trys pivoting at all the
		// parametric variables with non-zero coeffs for cs.
	virtual void LoadDesValCache();			// Used for Solve
     
	virtual void NewVariable(QcFloat &v)
		{ fVBiMap.Update(v, fTableau.NewVariable()); }

	bool IsDepVar(int vi) const;
		// Used by EditVars and Resolve optimised linkage.
	bool IsParOfInterest(int vi) const;

public:
	//-----------------------------------------------------------------------//
	// Utility functions.                                                    //
	//-----------------------------------------------------------------------//
	virtual void Print(ostream &os) const;
	virtual void Restart();
	  	// Erase everything ready to start afresh.

  	QcFloat &GetVariable(const char *n) const
  		{ return fVBiMap.Identifier(n); }

  	void GetVariableSet(vector<QcFloat> &vars) const
  		{ fVBiMap.GetVariableSet(vars); }

	const_var_iterator getVariables_begin() const
		{ return fVBiMap.getIdentifiers_begin(); }

	const_var_iterator getVariables_end() const
		{ return fVBiMap.getIdentifiers_end(); }

  	QcConstraint &GetConstraint(const char *n) const
  		{ return fOCBiMap.Identifier(n); }

private:
	bool doAddConstraint(QcConstraint &c, int hintIx);
};

inline QcLinEqSystem::QcLinEqSystem()
	: QcSolver(),
	  fVBiMap( fNotifier.GetVMap()),
	  fOCBiMap( fNotifier.GetOCMap()),
	  fTableau( *new QcLinEqTableau( 0, 0, fNotifier))
{
}

inline QcLinEqSystem::QcLinEqSystem(unsigned hintRows, unsigned hintCols)
	: QcSolver(), fVBiMap(fNotifier.GetVMap()),
	  fOCBiMap(fNotifier.GetOCMap()),
	  fTableau(*new QcLinEqTableau(hintRows, hintCols, fNotifier))
{
}

inline QcLinEqSystem::QcLinEqSystem(QcLinEqTableau &tab)
		: QcSolver(), fVBiMap(fNotifier.GetVMap()),
		fOCBiMap(fNotifier.GetOCMap()),
		fTableau(tab)
{
}

inline QcLinEqSystem::~QcLinEqSystem()
{
	delete &fTableau;
}

inline void QcLinEqSystem::AddVar(QcFloat &v)
{
	if (!fVBiMap.IdentifierPresent(v)) {
		NewVariable(v);
		dbg(assertInvar());
	}
}

inline bool QcLinEqSystem::ChangeConstraint(QcConstraint &c, numT rhs)
{
	int ix = fOCBiMap.safeIndex(c);
	if (ix < 0)
		return false; // TODO: perhaps use qcAssertExternalPre.
	fTableau.ChangeRHS(ix, rhs);
	// TODO: if (!fBatchAddConst) { maybe return false; }
	return true;
}

inline void QcLinEqSystem::Print(ostream &os) const
{
	QcSolver::Print(os);
	fNotifier.Print(os);
	os << endl;
	fTableau.Print(os);
}

inline void QcLinEqSystem::SuggestValue(QcFloat &v, numT desval)
{
	if (!fVBiMap.IdentifierPresent(v))
		throw QcWarning("SuggestValue: variable must be registered");
	else
		v.SuggestValue(desval);
}

#endif
