/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "LayoutOptimizer.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <AutoDeleter.h>


//#define TRACE_LAYOUT_OPTIMIZER	1
#if TRACE_LAYOUT_OPTIMIZER
#	define TRACE(format...)	printf(format)
#	define TRACE_ONLY(x)	x
#else
#	define TRACE(format...)
#	define TRACE_ONLY(x)
#endif
#define TRACE_ERROR(format...)	fprintf(stderr, format)

using std::nothrow;


/*!	\class BPrivate::Layout::LayoutOptimizer

	Given a set of layout constraints, a feasible solution, and a desired
	(non-)solution this class finds an optimal solution. The optimization
	criterion is to minimize the norm of the difference to the desired
	(non-)solution.

	It does so by implementing an active set method algorithm. The basic idea
	is to start with the subset of the constraints that are barely satisfied by
	the feasible solution, i.e. including all equality constraints and those
	inequality constraints that are still satisfied, if restricted to equality
	constraints. This set is called active set, the contained constraints active
	constraints.

	Considering all of the active constraints equality constraints a new
	solution is computed, which still satisfies all those equality constraints
	and is optimal with respect to the optimization criterion.

	If the new solution equals the previous one, we find the inequality
	constraint that, by keeping it in the active set, prevents us most from
	further optimizing the solution. If none really does, we're done, having
	found the globally optimal solution. Otherwise we remove the found
	constraint from the active set and try again.

	If the new solution does not equal the previous one, it might violate one
	or more of the inactive constraints. If that is the case, we add the
	most-violated constraint to the active set and adjust the new solution such
	that barely satisfies that constraint. Otherwise, we don't adjust the
	computed solution. With the adjusted respectively unadjusted solution
	we enter the next iteration, i.e. by computing a new optimal solution with
	respect to the active set.
*/


// #pragma mark - vector and matrix operations


// is_zero
static inline bool
is_zero(double* x, int n)
{
	for (int i = 0; i < n; i++) {
		if (!fuzzy_equals(x[i], 0))
			return false;
	}

	return true;
}


// add_vectors
static inline void
add_vectors(double* x, const double* y, int n)
{
	for (int i = 0; i < n; i++)
		x[i] += y[i];
}


// add_vectors_scaled
static inline void
add_vectors_scaled(double* x, const double* y, double scalar, int n)
{
	for (int i = 0; i < n; i++)
		x[i] += y[i] * scalar;
}


// negate_vector
static inline void
negate_vector(double* x, int n)
{
	for (int i = 0; i < n; i++)
		x[i] = -x[i];
}


// allocate_matrix
double**
allocate_matrix(int m, int n)
{
	double** matrix = new(nothrow) double*[m];
	if (!matrix)
		return NULL;

	double* values = new(nothrow) double[m * n];
	if (!values) {
		delete[] matrix;
		return NULL;
	}

	double* row = values;
	for (int i = 0; i < m; i++, row += n)
		matrix[i] = row;

	return matrix;
}


// free_matrix
void
free_matrix(double** matrix)
{
	if (matrix) {
		delete[] *matrix;
		delete[] matrix;
	}
}


// multiply_matrix_vector
/*!	y = Ax
	A: m x n matrix
*/
static inline void
multiply_matrix_vector(const double* const* A, const double* x, int m, int n,
	double* y)
{
	for (int i = 0; i < m; i++) {
		double sum = 0;
		for (int k = 0; k < n; k++)
			sum += A[i][k] * x[k];
		y[i] = sum;
	}
}


// multiply_matrices
/*!	c = a*b
*/
static void
multiply_matrices(const double* const* a, const double* const* b, double** c,
	int m, int n, int l)
{
	for (int i = 0; i < m; i++) {
		for (int j = 0; j < l; j++) {
			double sum = 0;
			for (int k = 0; k < n; k++)
				sum += a[i][k] * b[k][j];
			c[i][j] = sum;
		}	
	}
}


// transpose_matrix
static inline void
transpose_matrix(const double* const* A, double** Atrans, int m, int n)
{
	for (int i = 0; i < m; i++) {
		for (int k = 0; k < n; k++)
			Atrans[k][i] = A[i][k];
	}
}


// zero_matrix
void
zero_matrix(double** A, int m, int n)
{
	for (int i = 0; i < m; i++) {
		for (int k = 0; k < n; k++)
			A[i][k] = 0;
	}
}


// copy_matrix
void
copy_matrix(const double* const* A, double** B, int m, int n)
{
	for (int i = 0; i < m; i++) {
		for (int k = 0; k < n; k++)
			B[i][k] = A[i][k];
	}
}


static inline void
multiply_optimization_matrix_vector(const double* x, int n, double* y)
{
	// The matrix has the form:
	//  2 -1  0     ...   0  0
	// -1  2 -1  0  ...   .  .
	//  0 -1  2           .  .
	//  .  0     .        .  .
	//  .           .     0  0
	//  .              . -1  0
	//  0    ...    0 -1  2 -1
	//  0    ...         -1  1
	if (n == 1) {
		y[0] = x[0];
		return;
	}

	y[0] = 2 * x[0] - x[1];
	for (int i = 1; i < n - 1; i++)
		y[i] = 2 * x[i] - x[i - 1] - x[i + 1];
	y[n - 1] = x[n - 1] - x[n - 2];
}


static inline void
multiply_optimization_matrix_matrix(const double* const* A, int m, int n,
	double** B)
{
	if (m == 1) {
		memcpy(B[0], A[0], n * sizeof(double));
		return;
	}

	for (int k = 0; k < n; k++) {
		B[0][k] = 2 * A[0][k] - A[1][k];
		for (int i = 1; i < m - 1; i++)
			B[i][k] = 2 * A[i][k] - A[i - 1][k] - A[i + 1][k];
		B[m - 1][k] = A[m - 1][k] - A[m - 2][k];
	}
}


template<typename Type>
static inline void
swap(Type& a, Type& b)
{
	Type c = a;
	a = b;
	b = c;
}


// #pragma mark - algorithms

#if 0
static void
print_system(double** a, int n, double* b)
{
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			printf("%.1f ", a[i][j]);
		}
		printf("= %f\n", b[i]);
	}
}
#endif


static bool
solve(double** a, int n, double* b)
{
	// index array for row permutation
	// Note: We could eliminate it, if we would permutate the row pointers of a.
	int indices[n];
	for (int i = 0; i < n; i++)
		indices[i] = i;

	// forward elimination
	for (int i = 0; i < n - 1; i++) {
		// find pivot
		int pivot = i;
		double pivotValue = fabs(a[indices[i]][i]);
		for (int j = i + 1; j < n; j++) {
			int index = indices[j];
			double value = fabs(a[index][i]);
			if (value > pivotValue) {
				pivot = j;
				pivotValue = value;
			}
		}

		if (fuzzy_equals(pivotValue, 0)) {
			TRACE_ERROR("solve(): matrix is not regular\n");
			return false;
		}

		if (pivot != i) {
			swap(indices[i], indices[pivot]);
			swap(b[i], b[pivot]);
		}
		pivot = indices[i];

		// eliminate
		for (int j = i + 1; j < n; j++) {
			int index = indices[j];
			double q = -a[index][i] / a[pivot][i];
			a[index][i] = 0;
			for (int k = i + 1; k < n; k++)
				a[index][k] += a[pivot][k] * q;
			b[j] += b[i] * q;
		}
	}

	// backwards substitution
	for (int i = n - 1; i >= 0; i--) {
		int index = indices[i];
		double sum = b[i];
		for (int j = i + 1; j < n; j++)
			sum -= a[index][j] * b[j];

		if (fuzzy_equals(a[index][i], 0))
			return false;
		b[i] = sum / a[index][i];
	}

	return true;
}


int
compute_dependencies(double** a, int m, int n,
	bool* independent)
{
	// index array for row permutation
	// Note: We could eliminate it, if we would permutate the row pointers of a.
	int indices[m];
	for (int i = 0; i < m; i++)
		indices[i] = i;

	// forward elimination
	int iterations = (m > n ? n : m);
	int i = 0;
	int column = 0;
	for (; i < iterations && column < n; i++) {
		// find next pivot
		int pivot = i;
		do {
			double pivotValue = fabs(a[indices[i]][column]);
			for (int j = i + 1; j < m; j++) {
				int index = indices[j];
				double value = fabs(a[index][column]);
				if (value > pivotValue) {
					pivot = j;
					pivotValue = value;
				}
			}

			if (!fuzzy_equals(pivotValue, 0))
				break;

			column++;
		} while (column < n);

		if (column == n)
			break;

		if (pivot != i)
			swap(indices[i], indices[pivot]);
		pivot = indices[i];

		independent[pivot] = true;

		// eliminate
		for (int j = i + 1; j < m; j++) {
			int index = indices[j];
			double q = -a[index][column] / a[pivot][column];
			a[index][column] = 0;
			for (int k = column + 1; k < n; k++)
				a[index][k] += a[pivot][k] * q;
		}

		column++;
	}

	for (int j = i; j < m; j++)
		independent[indices[j]] = false;

	return i;
}


// remove_linearly_dependent_rows
static int
remove_linearly_dependent_rows(double** A, double** temp,
	bool* independentRows, int m, int n)
{
	// copy to temp
	copy_matrix(A, temp, m, n);

	int count = compute_dependencies(temp, m, n, independentRows);
	if (count == m)
		return count;

	// remove the rows
	int index = 0;
	for (int i = 0; i < m; i++) {
		if (independentRows[i]) {
			if (index < i) {
				for (int k = 0; k < n; k++)
					A[index][k] = A[i][k];
			}
			index++;
		}
	}

	return count;
}


/*!	QR decomposition using Householder transformations.
*/
static bool
qr_decomposition(double** a, int m, int n, double* d, double** q)
{
	if (m < n)
		return false;

	for (int j = 0; j < n; j++) {
		// inner product of the first vector x of the (j,j) minor
		double innerProductU = 0;
		for (int i = j + 1; i < m; i++)
			innerProductU = innerProductU + a[i][j] * a[i][j];
		double innerProduct = innerProductU + a[j][j] * a[j][j];
		if (fuzzy_equals(innerProduct, 0)) {
			TRACE_ERROR("qr_decomposition(): 0 column %d\n", j);
			return false;
		}

		// alpha (norm of x with opposite signedness of x_1) and thus r_{j,j}
		double alpha = (a[j][j] < 0 ? sqrt(innerProduct) : -sqrt(innerProduct));
		d[j] = alpha;

		double beta = 1 / (alpha * a[j][j] - innerProduct);

		// u = x - alpha * e_1
		// (u is a[j..n][j])
		a[j][j] -= alpha;

		// left-multiply A_k with Q_k, thus obtaining a row of R and the A_{k+1}
		// for the next iteration
		for (int k = j + 1; k < n; k++) {
			double sum = 0;
			for (int i = j; i < m; i++)
				sum += a[i][j] * a[i][k];
			sum *= beta;

			for (int i = j; i < m; i++)
				a[i][k] += a[i][j] * sum;
		}

		// v = u/|u|
		innerProductU += a[j][j] * a[j][j];
		double beta2 = -2 / innerProductU;

		// right-multiply Q with Q_k
		// Q_k = I - 2vv^T
		// Q * Q_k = Q - 2 * Q * vv^T
		if (j == 0) {
			for (int k = 0; k < m; k++) {
				for (int i = 0; i < m; i++)
					q[k][i] = beta2 * a[k][0] * a[i][0];

				q[k][k] += 1;
			}
		} else {
			for (int k = 0; k < m; k++) {
				double sum = 0;
				for (int i = j; i < m; i++)
					sum += q[k][i] * a[i][j];
				sum *= beta2;
	
				for (int i = j; i < m; i++)
					q[k][i] += sum * a[i][j];
			}
		}
	}

	return true;
}


// MatrixDeleter
struct MatrixDelete {
	inline void operator()(double** matrix)
	{
		free_matrix(matrix);
	}
};
typedef BPrivate::AutoDeleter<double*, MatrixDelete> MatrixDeleter;


// #pragma mark - LayoutOptimizer


// constructor
LayoutOptimizer::LayoutOptimizer(const ConstraintList& list,
	int32 variableCount)
	:
	fVariableCount(0),
	fTemp1(NULL),
	fTemp2(NULL),
	fZtrans(NULL),
	fQ(NULL),
	fSoftConstraints(NULL),
	fG(NULL),
	fDesired(NULL)
{
	SetConstraints(list, variableCount);
}


// destructor
LayoutOptimizer::~LayoutOptimizer()
{
	_MakeEmpty();
}


bool
LayoutOptimizer::SetConstraints(const ConstraintList& list, int32 variableCount)
{
	fConstraints = (ConstraintList*)&list;
	int32 constraintCount = fConstraints->CountItems();

	if (fVariableCount != variableCount) {
		_MakeEmpty();
		_Init(variableCount, constraintCount);
	}

	zero_matrix(fSoftConstraints, constraintCount, fVariableCount);
	double rightSide[constraintCount];
	// set up soft constraint matrix
	for (int32 c = 0; c < fConstraints->CountItems(); c++) {
		Constraint* constraint = fConstraints->ItemAt(c);
		if (!constraint->IsSoft()) {
			rightSide[c] = 0;
			continue;
		}
		double weight = 0;
		double negPenalty = constraint->PenaltyNeg();
		if (negPenalty > 0)
			weight += negPenalty;
		double posPenalty = constraint->PenaltyPos();
		if (posPenalty > 0)
			weight += posPenalty;
		if (negPenalty > 0 && posPenalty > 0)
			weight /= 2;
		
		rightSide[c] = _RightSide(constraint) * weight;
		SummandList* summands = constraint->LeftSide();
		for (int32 s = 0; s < summands->CountItems(); s++) {
			Summand* summand = summands->ItemAt(s);
			int32 variable = summand->Var()->Index();
			if (constraint->Op() == LinearProgramming::kLE)
				fSoftConstraints[c][variable] = -summand->Coeff();
			else
				fSoftConstraints[c][variable] = summand->Coeff();
			fSoftConstraints[c][variable] *= weight;
		}
	}

	// create G
	transpose_matrix(fSoftConstraints, fTemp1, constraintCount, fVariableCount);
	multiply_matrices(fTemp1, fSoftConstraints, fG, fVariableCount,
		constraintCount, constraintCount);

	// create d
	multiply_matrix_vector(fTemp1, rightSide, fVariableCount, constraintCount,
		fDesired);
	negate_vector(fDesired, fVariableCount);

	return true;
}


// InitCheck
status_t
LayoutOptimizer::InitCheck() const
{
	if (!fTemp1 || !fTemp2 || !fZtrans || !fQ || !fSoftConstraints || !fG
		|| !fDesired)
		return B_NO_MEMORY;
	return B_OK;
}


double
LayoutOptimizer::_ActualValue(Constraint* constraint, double* values) const
{
	SummandList* summands = constraint->LeftSide();
	double value = 0;
	for (int32 s = 0; s < summands->CountItems(); s++) {
		Summand* summand = summands->ItemAt(s);
		int32 variable = summand->Var()->Index();
		value += values[variable] * summand->Coeff();
	}
	if (constraint->Op() == LinearProgramming::kLE)
		return -value;
	return value;
}


double
LayoutOptimizer::_RightSide(Constraint* constraint)
{
	if (constraint->Op() == LinearProgramming::kLE)
		return -constraint->RightSide();
	return constraint->RightSide();
}


void
LayoutOptimizer::_MakeEmpty()
{
	free_matrix(fTemp1);
	free_matrix(fTemp2);
	free_matrix(fZtrans);
	free_matrix(fSoftConstraints);
	free_matrix(fQ);
	free_matrix(fG);

	delete[] fDesired;
}


void
LayoutOptimizer::_Init(int32 variableCount, int32 nConstraints)
{
	fVariableCount = variableCount;

	fTemp1 = allocate_matrix(nConstraints, nConstraints);
	fTemp2 = allocate_matrix(nConstraints, nConstraints);
	fZtrans = allocate_matrix(nConstraints, fVariableCount);
	fSoftConstraints = allocate_matrix(nConstraints, fVariableCount);
	fQ = allocate_matrix(nConstraints, fVariableCount);
	fG = allocate_matrix(nConstraints, nConstraints);

	fDesired = new(std::nothrow) double[fVariableCount];
}


// Solve
/*!	Solves the quadratic program (QP) given by the constraints added via
	AddConstraint(), the additional constraint \sum_{i=0}^{n-1} x_i = size,
	and the optimization criterion to minimize
	\sum_{i=0}^{n-1} (x_i - desired[i])^2.
	The \a values array must contain a feasible solution when called and will
	be overwritten with the optimial solution the method computes.
*/
bool
LayoutOptimizer::Solve(double* values)
{
	if (values == NULL)
		return false;

	int32 constraintCount = fConstraints->CountItems();

	// allocate the active constraint matrix and its transposed matrix
	fActiveMatrix = allocate_matrix(constraintCount, fVariableCount);
	fActiveMatrixTemp = allocate_matrix(constraintCount, fVariableCount);
	MatrixDeleter _(fActiveMatrix);
	MatrixDeleter _2(fActiveMatrixTemp);
	if (!fActiveMatrix || !fActiveMatrixTemp)
		return false;

	bool success = _Solve(values);
	return success;
}


// _Solve
bool
LayoutOptimizer::_Solve(double* values)
{
	int32 constraintCount = fConstraints->CountItems();

TRACE_ONLY(
	TRACE("constraints:\n");
	for (int32 i = 0; i < constraintCount; i++) {
		TRACE(" %-2ld:  ", i);
		fConstraints->ItemAt(i)->PrintToStream();
	}
)

	// our QP is supposed to be in this form:
	//   min_x 1/2x^TGx + x^Td
	//   s.t. a_i^Tx = b_i,  i \in E
	//        a_i^Tx >= b_i, i \in I

	// init our initial x
	double x[fVariableCount];
	for (int i = 0; i < fVariableCount; i++)
		x[i] = values[i];

	// init d
	// Note that the values of d and of G result from rewriting the
	// ||x - desired|| we actually want to minimize.
	double d[fVariableCount];
	for (int i = 0; i < fVariableCount; i++)
		d[i] = fDesired[i];

	// init active set
	ConstraintList activeConstraints(constraintCount);

	for (int32 i = 0; i < constraintCount; i++) {
		Constraint* constraint = fConstraints->ItemAt(i);
		if (constraint->IsSoft())
			continue;
		double actualValue = _ActualValue(constraint, x);
		TRACE("constraint %ld: actual: %f  constraint: %f\n", i, actualValue,
			_RightSide(constraint));
		if (fuzzy_equals(actualValue, _RightSide(constraint)))
			activeConstraints.AddItem(constraint);
	}

	// The main loop: Each iteration we try to get closer to the optimum
	// solution. We compute a vector p that brings our x closer to the optimum.
	// We do that by computing the QP resulting from our active constraint set,
	// W^k. Afterward each iteration we adjust the active set.
TRACE_ONLY(int iteration = 0;)
	while (true) {
TRACE_ONLY(
		TRACE("\n[iteration %d]\n", iteration++);
		TRACE("active set:\n");
		for (int32 i = 0; i < activeConstraints.CountItems(); i++) {
			TRACE("  ");
			activeConstraints.ItemAt(i)->PrintToStream();
		}
)

		// solve the QP:
		//   min_p 1/2p^TGp + g_k^Tp
		//   s.t. a_i^Tp = 0
		//   with a_i \in activeConstraints
		//        g_k = Gx_k + d
		//        p = x - x_k

		int32 activeCount = activeConstraints.CountItems();
		if (activeCount == 0) {
			TRACE_ERROR("Solve(): Error: No more active constraints!\n");
			return false;
		}

		// construct a matrix from the active constraints
		int am = activeCount;
		const int an = fVariableCount;
		bool independentRows[activeCount];
		zero_matrix(fActiveMatrix, am, an);

		for (int32 i = 0; i < activeCount; i++) {
			Constraint* constraint = activeConstraints.ItemAt(i);
			if (constraint->IsSoft())
				continue;
			SummandList* summands = constraint->LeftSide();
			for (int32 s = 0; s < summands->CountItems(); s++) {
				Summand* summand = summands->ItemAt(s);
				int32 variable = summand->Var()->Index();
				if (constraint->Op() == LinearProgramming::kLE)
					fActiveMatrix[i][variable] = -summand->Coeff();
				else
					fActiveMatrix[i][variable] = summand->Coeff();
			}
		}

// TODO: The fActiveMatrix is sparse (max 2 entries per row). There should be
// some room for optimization.
		am = remove_linearly_dependent_rows(fActiveMatrix, fActiveMatrixTemp,
			independentRows, am, an);

		// gxd = G * x + d
		double gxd[fVariableCount];
		multiply_matrix_vector(fG, x, fVariableCount, fVariableCount, gxd);
		add_vectors(gxd, d, fVariableCount);

		double p[fVariableCount];
		if (!_SolveSubProblem(gxd, am, p))
			return false;

		if (is_zero(p, fVariableCount)) {
			// compute Lagrange multipliers lambda_i
			// if lambda_i >= 0 for all i \in W^k \union inequality constraints,
			// then we're done.
			// Otherwise remove the constraint with the smallest lambda_i
			// from the active set.
			// The derivation of the Lagrangian yields:
			//   \sum_{i \in W^k}(lambda_ia_i) = Gx_k + d
			// Which is an system we can solve:
			//   A^Tlambda = Gx_k + d

			// A^T is over-determined, hence we need to reduce the number of
			// rows before we can solve it.
			bool independentColumns[an];
			double** aa = fTemp1;
			transpose_matrix(fActiveMatrix, aa, am, an);
			const int aam = remove_linearly_dependent_rows(aa, fTemp2,
				independentColumns, an, am);
			const int aan = am;
			if (aam != aan) {
				// This should not happen, since A has full row rank.
				TRACE_ERROR("Solve(): Transposed A has less linear independent "
					"rows than it has columns!\n");
				return false;
			}

			// also reduce the number of rows on the right hand side
			double lambda[aam];
			int index = 0;
			for (int i = 0; i < an; i++) {
				if (independentColumns[i])
					lambda[index++] = gxd[i];
			}

			bool success = solve(aa, aam, lambda);
			if (!success) {
				// Impossible, since we've removed all linearly dependent rows.
				TRACE_ERROR("Solve(): Failed to compute lambda!\n");
				return false;
			}

			// find min lambda_i (only, if it's < 0, though)
			double minLambda = 0;
			int minIndex = -1;
			index = 0;
			for (int i = 0; i < activeCount; i++) {
				if (independentRows[i]) {
					Constraint* constraint = activeConstraints.ItemAt(i);
					if (constraint->Op() != LinearProgramming::kEQ) {
						if (lambda[index] < minLambda) {
							minLambda = lambda[index];
							minIndex = i;
						}
					}

					index++;
				}
			}

			// if the min lambda is >= 0, we're done
			if (minIndex < 0 || fuzzy_equals(minLambda, 0)) {
				_SetResult(x, values);
				return true;
			}

			// remove i from the active set
			activeConstraints.RemoveItemAt(minIndex);
		} else {
			// compute alpha_k
			double alpha = 1;
			int barrier = -1;
			// if alpha_k < 1, add a barrier constraint to W^k
			for (int32 i = 0; i < constraintCount; i++) {
				Constraint* constraint = fConstraints->ItemAt(i);
				if (activeConstraints.HasItem(constraint))
					continue;

				double divider = _ActualValue(constraint, p);
				if (divider > 0 || fuzzy_equals(divider, 0))
					continue;

				// (b_i - a_i^Tx_k) / a_i^Tp_k
				double alphaI = _RightSide(constraint)
					- _ActualValue(constraint, x);
				alphaI /= divider;
				if (alphaI < alpha) {
					alpha = alphaI;
					barrier = i;
				}
			}
			TRACE("alpha: %f, barrier: %d\n", alpha, barrier);

			if (alpha < 1)
				activeConstraints.AddItem(fConstraints->ItemAt(barrier));

			// x += p * alpha;
			add_vectors_scaled(x, p, alpha, fVariableCount);
		}
	}
}


bool
LayoutOptimizer::_SolveSubProblem(const double* d, int am, double* p)
{
	// We have to solve the QP subproblem:
	//   min_p 1/2p^TGp + d^Tp
	//   s.t. a_i^Tp = 0
	//   with a_i \in activeConstraints
	//
	// We use the null space method, i.e. we find matrices Y and Z, such that
	// AZ = 0 and [Y Z] is regular. Then with
	//   p = Yp_Y + Zp_z
	// we get
	//   p_Y = 0
	// and
	//  (Z^TGZ)p_Z = -(Z^TYp_Y + Z^Tg) = -Z^Td
	// which is a linear equation system, which we can solve.

	const int an = fVariableCount;

	// we get Y and Z by QR decomposition of A^T
	double tempD[am];
	double** const Q = fQ;
	transpose_matrix(fActiveMatrix, fTemp1, am, an);
	bool success = qr_decomposition(fTemp1, an, am, tempD, Q);
	if (!success) {
		TRACE_ERROR("Solve(): QR decomposition failed!\n");
		return false;
	}

	// Z is the (1, m + 1) minor of Q
	const int zm = an;
	const int zn = an - am;

	double* Z[zm];
	for (int i = 0; i < zm; i++)
		Z[i] = Q[i] + am;

	// solve (Z^TGZ)p_Z = -Z^Td

	// Z^T
	transpose_matrix(Z, fZtrans, zm, zn);
	// rhs: -Z^T * d;
	double pz[zm];
	multiply_matrix_vector(fZtrans, d, zn, zm, pz);
	negate_vector(pz, zn);

	// fTemp2 = Ztrans * G * Z
	multiply_matrices(fG, Z, fTemp1, zm, fVariableCount, zn);
	multiply_matrices(fZtrans, fTemp1, fTemp2, zn, zm, zn);

	success = solve(fTemp2, zn, pz);
	if (!success) {
		TRACE_ERROR("Solve(): Failed to solve() system for p_Z\n");
		return false;
	}

	// p = Z * pz;
	multiply_matrix_vector(Z, pz, zm, zn, p);

	return true;
}


// _SetResult
void
LayoutOptimizer::_SetResult(const double* x, double* values)
{
	for (int i = 0; i < fVariableCount; i++)
		values[i] = x[i];
}
