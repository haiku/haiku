// $Id: QcCassSolver.cc,v 1.14 2001/01/04 05:20:11 pmoulder Exp $

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

#include "qoca/QcDefines.hh"
#include "qoca/QcCassSolver.hh"
#include "qoca/QcStructVarIndexIterator.hh"
#include "qoca/QcVariableIndexIterator.hh"

QcCassSolver::~QcCassSolver()
{
	// Deallocate memory allocated to Cassowary stay constraints.
	TStayMap::iterator sIt = fStayMap.begin();

	while (sIt != fStayMap.end()) {
         delete (*sIt).second;
         sIt++;
	}
}

bool QcCassSolver::AddConstraint(QcConstraint &c)
{
	const QcLinPoly &lp = c.LinPoly();
	const QcLinPoly::TTermsVector &terms = lp.GetTerms();

	// For each QcFloat, check if it has already been added as stay
	// constraint.
	for (unsigned int i = 0; i < terms.size(); i++) {
		QcFloat &v = terms[i]->GetVariable();
		if (!IsRegistered(v))
			AddVar(v);
	}

	bool success = QcDelLinInEqSystem::AddConstraint(c);
	if (success) {
#if qcCheckPost
		addCheckedConstraint( c.pointer());
#endif

		if (fAutoSolve) 
			Solve();
	}
	return success;
}

bool QcCassSolver::AddConstraint(QcConstraint &c, QcFloat &hint)
{
	const QcLinPoly &lp = c.LinPoly();
	const QcLinPoly::TTermsVector &terms = lp.GetTerms();

	// For each QcFloat, check if it has already been added as stay
	// constraint.
	for (unsigned int i = 0; i < terms.size(); i++) {
		QcFloat &v = terms[i]->GetVariable();
		if (!IsRegistered(v))
			AddVar(v);
	}

	bool success = QcDelLinInEqSystem::AddConstraint(c, hint);
	if (success) {
#if qcCheckPost
		addCheckedConstraint( c.pointer());
#endif
		
		if (fAutoSolve) 
			Solve();
	}
	return success;
}

void QcCassSolver::AddEditVar(QcFloat &v)
{
	QcSolver::AddEditVar(v);
	TStayMap::iterator sIt = fStayMap.find(v);
	if (sIt == fStayMap.end()) {
		throw QcWarning(
			"QcCassSolver::AddEditVar - adding an unregistered variable");
		return;
	}

	QcCassConstraint *stay = (*sIt).second;
	stay->fDvp.SetToEditWeight();
	stay->fDvm.SetToEditWeight();
	fEditMap.push_back(stay);
	fEditVarsSetup = false;
}

void QcCassSolver::AddVar(QcFloat &v)
{
	if (IsRegistered(v))
		return;

	QcDelLinInEqSystem::AddVar(v);
	QcFloat dvp("", 0.0, v.StayWeight(), v.EditWeight(), true);
	QcFloat dvm("", 0.0, v.StayWeight(), v.EditWeight(), true);
	fVBiMap.Update(dvp, fTableau.AddError());
	fVBiMap.Update(dvm, fTableau.AddError());

	QcConstraint stay = ((-1 * v) + (1 * dvp) + (-1 * dvm)
			     == -v.DesireValue());
	dbg(bool consistent_stay_constraint =)
	  QcDelLinInEqSystem::AddConstraint( stay);
	assert( consistent_stay_constraint);

	QcCassConstraint *cass = new QcCassConstraint( v, stay, dvp, dvm);
	fStayMap[v] = cass;
	fStaySeqMap.push_back( cass);

	qcAssertExternalPost( IsRegistered( v));
}

void QcCassSolver::BeginAddConstraint()
{
	throw QcWarning("Warning: QcCassSolver::BeginAddConstraint() not implemented yet");
}

void QcCassSolver::BeginEdit()
{
	if (!fEditVarsSetup) {
		if (fTableau.getNRestrictedRows() != 0) {
			InitObjective();
			SimplexII();
		}
		fEditVarsSetup = true;
	}
}

bool QcCassSolver::EndAddConstraint()
{
	throw QcWarning("Warning: QcCassSolver::EndAddConstraint() not implemented yet");
}

bool QcCassSolver::Reset()
{
  cerr << "Warning: QcCassSolver::Reset() not implemented yet\n";
  return false;
}

void QcCassSolver::EndEdit()
{
  for (TEditMap::iterator eIt = fEditMap.begin();
       eIt != fEditMap.end();
       eIt++)
    {
      QcCassConstraint *edit = (*eIt);
      edit->fDvp.SetToStayWeight();
      edit->fDvm.SetToStayWeight();
    }
  fEditMap.clear();

  // Call RestDesVal on things.
  for (TStaySeqMap::iterator sIt = fStaySeqMap.begin();
       sIt != fStaySeqMap.end();
       sIt++)
    {
      QcCassConstraint *stay = (*sIt);
      unsigned rowIx = fOCBiMap.Index( stay->fConstraint);
      /* TODO: Do more work on keeping fTableau RHS in sync with user variable
	 goal value.  Then we can make ChangeRHS conditional on the return value
	 of RestDesVal.

	 Also makes for better behaviour when user does suggestValue on a
	 non-edit-variable. */
      stay->fVariable.RestDesVal();
      fTableau.ChangeRHS( rowIx,
			  -stay->fVariable.DesireValue());
    }

  fEditVarsSetup = false;
  QcSolver::EndEdit();
}

void QcCassSolver::InitObjective()
{
	QcVariableIndexIterator vIt(fTableau);

	while (!vIt.AtEnd()) {
		if (fTableau.IsError(vIt.getIndex())) {
			QcFloat &v = fVBiMap.Identifier(vIt.getIndex());
			fTableau.SetObjValue(vIt.getIndex(), v.Weight());
		} else
			fTableau.SetObjValue(vIt.getIndex(), 0.0);

		vIt.Increment();
	}

	#ifdef qcSafetyChecks
	for (unsigned i = 0; i < fTableau.getNColumns(); i++)
		if (fTableau.GetObjValue( i) != 0.0)
			assert( fTableau.IsError( i));
	#endif

	fTableau.EliminateObjective();
}

void QcCassSolver::LoadDesValCache()
{
  QcStructVarIndexIterator vIt (fTableau);
  while (!vIt.AtEnd())
    {
      unsigned vi = vIt.getIndex();
      if (!fTableau.IsSlack (vi))
	fTableau.SetDesireValue (vi, fVBiMap.Identifier(vi).DesireValue());
      vIt.Increment();
    }
}

void QcCassSolver::Print(ostream &os) const
{
	QcDelLinInEqSystem::Print(os);
	os << "Stay constraints: ";

	TStaySeqMap::const_iterator sIt = fStaySeqMap.begin();

	while (sIt != fStaySeqMap.end()) {
    		(*sIt)->Print(os);
    		os << endl;
         	sIt++;
	}

	os << endl;
	os << "Edit constraints: ";

	TEditMap::const_iterator eIt = fEditMap.begin();

	while (eIt != fEditMap.end()) {
       	(*eIt)->Print(os);
		os << endl;
		eIt++;
	}

	os << endl;
}

void QcCassSolver::RawSolve()
{
  LoadDesValCache();

  for (QcStructVarIndexIterator vIt (fTableau);
       !vIt.AtEnd();
       vIt.Increment())
    {
      unsigned vi = vIt.getIndex();
      if (fTableau.IsSlack (vi))
	continue;

      QcFloat &v = fVBiMap.Identifier (vi);
      if (fTableau.IsBasic (vi))
	{
	  int ci = fTableau.IsBasicIn (vi);
	  assert (ci != QcTableau::fInvalidConstraintIndex);
	  v.SetValue (fTableau.GetRHS (ci));
	}
      else
	v.SetVariable();
    }
}

bool QcCassSolver::RemoveVar(QcFloat &v)
{
	TStayMap::iterator sIt = fStayMap.find(v);

	// If the variable to be deleted is not in the solver.
	if (sIt == fStayMap.end()) {
		throw QcWarning(
          	"QcCassSolver::RemoveVar - removing an unregistered variable");
		return false;
	}

	QcCassConstraint *stay = (*sIt).second;

	// Remove the stay constraint permanently since the associated variables
	// is removed. Note that the error variables are not removed here, but
	// through the LinInEqTableau class.
	QcDelLinInEqSystem::RemoveConstraint(stay->fConstraint);
 	QcDelLinInEqSystem::RemoveVar(stay->fVariable);

	// Remove the Cassowary constraint from the stay sequential map.
	TStaySeqMap::iterator ssIt = fStaySeqMap.begin();

	while (ssIt != fStaySeqMap.end()) {
    		if ((*ssIt) == stay) {
         		fStaySeqMap.erase(ssIt);
			break;
		}
		ssIt++;
	}

	// Remove the Cassowary constraint from stay map.
	delete stay;
	fStayMap.erase(sIt);

	return true;
}

void QcCassSolver::Resolve()
{
	if (!fEditVarsSetup)
		BeginEdit();

	TEditMap::iterator eIt = fEditMap.begin();

	while (eIt != fEditMap.end()) {
		QcCassConstraint *edit = (*eIt);
		fTableau.ChangeRHS(fOCBiMap.Index(edit->fConstraint),
				   -edit->fVariable.DesireValue());
         	eIt++;
	}

	DualSimplexII();
	RawSolve();

#if qcCheckPost
	checkSatisfied();
#endif
}

void QcCassSolver::Solve()
{
	InitObjective();
	SimplexII();
	RawSolve();

	TStaySeqMap::iterator sIt = fStaySeqMap.begin();

	while (sIt != fStaySeqMap.end()) {
		QcCassConstraint *stay = (*sIt);
		fTableau.ChangeRHS(fOCBiMap.Index(stay->fConstraint),
				   -stay->fVariable.Value());
		sIt++;
	}

#if qcCheckPost
	checkSatisfied();
#endif
}

void QcCassSolver::Restart()
{
	QcDelLinInEqSystem::Restart();
	fStayMap.clear();
	fStaySeqMap.clear();
	fEditMap.clear();
}


