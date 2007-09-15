#include <qoca/QcUtility.hh>

/* Needed for GCC4, avoids undefined references */

double const QcUtility::qcEps; //     = _QC_EPS;
double const QcUtility::qcNearEps; // = 9.313225746154785e-10;   // 2 ** -30
double const QcUtility::qcVaguelyNearEps; // = 9.5367431640625e-7; // 2 ** -20
double const QcUtility::qcMaxZeroVal; // = _QC_EPS * (1 - DBL_EPSILON / 2);

