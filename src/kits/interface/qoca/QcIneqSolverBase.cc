// Generated automatically from QcIneqSolverBase.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcIneqSolverBase.hh"
#line 1 "QcIneqSolverBase.ch"

#include <qoca/QcSolver.hh>







#line 80 "QcIneqSolverBase.ch"
//-----------------------------------------------------------------------//
// Constraint management methods                                         //
//-----------------------------------------------------------------------//


bool
QcIneqSolverBase::AddConstraint(QcConstraint &c)
{
  bool success = fSystem.AddConstraint( c);
  if (success)
    {
#if qcCheckPost
      addCheckedConstraint( c.pointer());
#endif
      if (fAutoSolve)
	Solve();
    }
  return success;
}


bool
QcIneqSolverBase::AddConstraint(QcConstraint &c, QcFloat &hint)
{
  bool success = fSystem.AddConstraint( c, hint);
  if (success)
    {
#if qcCheckPost
      addCheckedConstraint( c.pointer());
#endif
      if (fAutoSolve)
	Solve();
    }
  return success;
}


#line 131 "QcIneqSolverBase.ch"
bool
QcIneqSolverBase::ChangeConstraint(QcConstraint &oldc, numT rhs)
{
  bool success = fSystem.ChangeConstraint( oldc, rhs);
#if qcCheckPost
  if (success)
    changeCheckedConstraint( oldc.pointer(), rhs);
#endif
  return success;
}


bool
QcIneqSolverBase::RemoveConstraint(QcConstraint &c)
{
  bool success = fSystem.RemoveConstraint( c);
  if (success)
    {
#if qcCheckPost
      removeCheckedConstraint( c.pointer());
#endif
      if (fAutoSolve)
	Solve();
    }
  return success;
}


#line 173 "QcIneqSolverBase.ch"
/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
