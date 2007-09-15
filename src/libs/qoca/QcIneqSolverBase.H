// Generated automatically from QcIneqSolverBase.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcIneqSolverBaseDCL
#define QcIneqSolverBaseDCL
#include <qoca/QcSolver.H>

#include <qoca/QcDelLinInEqSystem.hh>

class QcIneqSolverBase
  : public QcSolver
{
public:

inline
#line 18 "QcIneqSolverBase.ch"
QcIneqSolverBase() 
  : QcSolver(),
    fSystem()
{
	;
}

inline
#line 26 "QcIneqSolverBase.ch"
QcIneqSolverBase(unsigned hintNumConstraints, unsigned hintNumVariables)
  : QcSolver(),
    fSystem( hintNumConstraints, hintNumVariables)
{
	;
}


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



//-----------------------------------------------------------------------//
// Constraint management methods                                         //
//-----------------------------------------------------------------------//


virtual bool
AddConstraint(QcConstraint &c);
virtual bool
AddConstraint(QcConstraint &c, QcFloat &hint);
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



virtual bool
ChangeConstraint(QcConstraint &oldc, numT rhs);
virtual bool
RemoveConstraint(QcConstraint &c);
virtual bool
Reset()
{
  return fSystem.Reset();
}




protected:
  QcDelLinInEqSystem fSystem;

};

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcIneqSolverBaseDCL */
