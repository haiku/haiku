//o[d] #include <qoca/QcSolver.H>
//o[ci] #include <qoca/QcSolver.hh>
//o[d] #include <qoca/QcDelLinInEqSystem.hh>

//ho class QcIneqSolverBase
//ho   : public QcSolver
//ho {
//ho public:

inline
QcIneqSolverBase()
  : QcSolver(),
    fSystem()
{
}


inline
QcIneqSolverBase(unsigned hintNumConstraints, unsigned hintNumVariables)
  : QcSolver(),
    fSystem( hintNumConstraints, hintNumVariables)
{
}

//begin ho
virtual ~QcIneqSolverBase()
{
}

//-----------------------------------------------------------------------//
// Variable management methods                                           //
//-----------------------------------------------------------------------//
virtual void
AddVar(QcFloat &v)
{
  fSystem.AddVar( v);
}

virtual void
SuggestValue(QcFloat &v, numT desval)
{
  fSystem.SuggestValue( v, desval);
}

virtual bool
RemoveVar(QcFloat &v)
{
  return fSystem.RemoveVar( v);
}

virtual void
RestSolver()
{
  fSystem.RestSolver();
}

//-----------------------------------------------------------------------//
// Enquiry functions for variables                                       //
//-----------------------------------------------------------------------//
virtual bool
IsRegistered(QcFloat const &v) const
{
  return fSystem.IsRegistered( v);
}

virtual bool
IsFree(QcFloat const &v) const
{
  return fSystem.IsFree( v);
}

virtual bool
IsBasic(QcFloat const &v) const
{
  return fSystem.IsBasic( v);
}
//end ho


//-----------------------------------------------------------------------//
// Constraint management methods                                         //
//-----------------------------------------------------------------------//

//cf
virtual bool
AddConstraint(QcConstraint &c)
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

//cf
virtual bool
AddConstraint(QcConstraint &c, QcFloat &hint)
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

//begin ho
virtual void
BeginAddConstraint()
{
  fSystem.BeginAddConstraint();
}

virtual bool
EndAddConstraint()
{
  return fSystem.EndAddConstraint();
}
//end ho

//cf
virtual bool
ChangeConstraint(QcConstraint &oldc, numT rhs)
{
  bool success = fSystem.ChangeConstraint( oldc, rhs);
#if qcCheckPost
  if (success)
    changeCheckedConstraint( oldc.pointer(), rhs);
#endif
  return success;
}

//cf
virtual bool
RemoveConstraint(QcConstraint &c)
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

//begin ho
virtual bool
Reset()
{
  return fSystem.Reset();
}
//end ho


//begin ho
protected:
  QcDelLinInEqSystem fSystem;
//end ho
//ho };

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
