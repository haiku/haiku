// Generated automatically from QcIneqSolverBase.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcIneqSolverBase.H"
#ifndef QcIneqSolverBaseIFN
#define QcIneqSolverBaseIFN
#line 1 "QcIneqSolverBase.ch"

#include <qoca/QcSolver.hh>







inline
QcIneqSolverBase::QcIneqSolverBase()
  : QcSolver(),
    fSystem()
{
}


inline
QcIneqSolverBase::QcIneqSolverBase(unsigned hintNumConstraints, unsigned hintNumVariables)
  : QcSolver(),
    fSystem( hintNumConstraints, hintNumVariables)
{
}


#line 80 "QcIneqSolverBase.ch"
//-----------------------------------------------------------------------//
// Constraint management methods                                         //
//-----------------------------------------------------------------------//


#line 173 "QcIneqSolverBase.ch"
/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcIneqSolverBaseIFN */
