/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ConstraintSolverLayouter.h"

#include <cmath>
#include <cstring>

#include <LayoutUtils.h>
#include <Size.h>
#include <String.h>

#include "SimpleLayouter.h"

// Since the qoca headers are DEBUG and NDEBUG conditional, we get
// incompatibilities when compiling this file with debugging and qoca
// without. So we consistently don't allow qoca to be compiled with
// debugging and undefine the macros before using the includes.
#undef DEBUG
#undef NDEBUG

#include <qoca/QcConstraint.hh>
#include <qoca/QcFloat.hh>
#include <qoca/QcLinInEqSolver.hh>
#include <qoca/QcLinPoly.hh>



// This monster is a Layouter implementation similar to SimpleLayouter with
// the difference that it accepts min/max constraints over several elements,
// i.e. not only constraints of the form x_i <=/>= a, but also of the form
// x_i + x_{i+1} + ... x_{i+m} <=/>= a. This adds significant complexity,
// hence we employ a constraint solver, QOCA's QcLinInEqSolver, to find
// solutions.
//
// Two main problems remain, though:
// 1) QOCA supplies us with a real value solution, while we actually need an
//    integer solution. Generally finding an optimal integer solution for
//	  a inequation system is at least NP hard (AFAIK even undecidable, if
//	  the variables are not restricted to >= 0). Fortunately our system has
//	  special properties that make things less complex. I haven't found
//	  anything related in the literature, but I've designed an algorithm that
//	  derives an integer solution from a real value solution. I wasn't able
//	  to prove, that there's always an integer solution given there's a 
//	  real value solution (though I've the feeling that this is the case),
//	  but if there is one, the algorithm should be able to find it.
// 2) Constraints given to the layouter may be contradictory. If that is the
//	  case, we try to relax constraints such that they become
//	  non-contradictory again. We use heuristics to do this, like preferring
//	  min constraints over max constraints, ones with few involved variables
//	  over ones with many involved variables.


//#define ERROR_MESSAGE(message...) {}
#define ERROR_MESSAGE(message...) { printf(message); }


// no lround() under BeOS R5 x86
#ifdef HAIKU_TARGET_PLATFORM_LIBBE_TEST
#	define lround(x)	(long)floor((x) + 0.5)
#endif


// Set
class ConstraintSolverLayouter::Set {
public:
	Set()
		: fElements()
	{
	}

	BList& ToList()
	{
		return fElements;
	}

	const BList& ToList() const
	{
		return fElements;
	}

	bool Add(void* element)
	{
		if (fElements.HasItem(element))
			return false;

		fElements.AddItem(element);
		return true;
	}

	void AddAll(const Set& other)
	{
		AddAll(other.ToList());
	}

	void AddAll(const BList& other)
	{
		int32 count = other.CountItems();
		for (int32 i = 0; i < count; i++)
			Add(other.ItemAt(i));
	}

	bool Remove(void* element)
	{
		return fElements.RemoveItem(element);
	}

	void MakeEmpty()
	{
		fElements.MakeEmpty();
	}

	bool Contains(void* element) const
	{
		return fElements.HasItem(element);
	}

	int32 Size() const
	{
		return fElements.CountItems();
	}

	bool IsEmpty() const
	{
		return fElements.IsEmpty();
	}

	void* Get(int32 index) const
	{
		return fElements.ItemAt(index);
	}

private:
	BList	fElements;
};


// Variable
class ConstraintSolverLayouter::Variable {
public:
	Variable()
		: fQocaVariable(NULL),
		  fValue(0),
		  fIndex(-1)
	{
	}

	Variable(const char* name)
		: fQocaVariable(new QcFloat(name, true)),
		  fValue(0),
		  fIndex(-1)
	{
	}

	Variable(int32 index)
		: fQocaVariable(NULL),
		  fValue(0),
		  fIndex(index)
	{
		Init(index);
	}

	~Variable()
	{
		delete fQocaVariable;
	}

	void Init(int32 index)
	{
		delete fQocaVariable;

		fIndex = index;

		BString name("x");
		name << index;
		fQocaVariable = new QcFloat(name.String(), true);
	}

	int32 Index() const
	{
		return fIndex;
	}
	
	QcFloat* QocaVariable() const
	{
		return fQocaVariable;
	}

	double SolvedValue() const
	{
		return fQocaVariable->Value();
	}
	
	int32 IntValue() const
	{
		return fValue;
	}

	void SetIntValue(int32 value)
	{
		fValue = value;
	}

private:
	QcFloat*	fQocaVariable;
	int32		fValue;
	int32		fIndex;
};


// Constraint
class ConstraintSolverLayouter::Constraint {
public:
	Constraint(ConstraintSolverLayouter* layouter,
			int32 first, int32 last, int32 value)
		: fLayouter(layouter),
		  fFirst(first),
		  fLast(last),
		  fOriginalValue(-1),	// so SetOriginalValue() detects a difference
		  fValue(-1),			// so SetValue() detects a difference
		  fPolynom(NULL),
		  fQocaConstraint(NULL),
		  fAdded(false),
		  fAlgorithmData(0)

	{
		fPolynom = new QcLinPoly();
		for (int i = first; i <= last; i++)
			fPolynom->Push(1, *fLayouter->fVariables[i].QocaVariable());
		fQocaConstraint = new QcConstraint();
	}


	virtual ~Constraint()
	{
		delete fPolynom;
		delete fQocaConstraint;
	}

	virtual Constraint* CloneConstraint(ConstraintSolverLayouter* layouter) = 0;
	
	int32 First() const
	{
		return fFirst;
	}

	int32 Last() const
	{
		return fLast;
	}

	int32 OriginalValue() const
	{
		return fOriginalValue;
	}
	
	void SetOriginalValue(int32 value)
	{
		if (value != fOriginalValue) {
			fOriginalValue = value;
			SetValue(value);
		}
	}

	virtual void RestrictOriginalValue(int32 value) = 0;
	
	int32 Value() const
	{
		return fValue;
	}
	
	void SetValue(int32 value)
	{
		if (value == fValue)
			return;

		fValue = value;

		UpdateQocaConstraint();
	}

	int32 AlgorithmData() const
	{
		return fAlgorithmData;
	}

	void SetAlgorithmData(int32 algorithmData)
	{
		fAlgorithmData = algorithmData;
	}
	
	QcConstraint* QocaConstraint() const
	{
		return fQocaConstraint;
	}

	bool IsAdded() const
	{
		return fAdded;
	}

	void SetAdded(bool added)
	{
		fAdded = added;
	}

	bool IsSatisfied() const
	{
		return (ComputeSatisfactionDistance() <= 0);
	}
	
	/**
	 * Returns the distance the sum value need to change for the constraint to
	 * get satisfied.
	 * 
	 * A negative return value means that the constraint is satisfied and its
	 * absolute value is the distance the sum can change to be still satisfied.
	 * 
	 * @return the distance the sum value need to change for the constraint to
	 * get satisfied.
	 */
	virtual int32 ComputeSatisfactionDistance() const = 0;

	virtual void UpdateQocaConstraint() = 0;
	
	int32 ComputeVariableSum() const
	{
		int sum = 0;
		for (int i = fFirst; i <= fLast; i++)
			sum += fLayouter->fVariables[i].IntValue();

		return sum;
	}


protected:
	ConstraintSolverLayouter* fLayouter;

	int32			fFirst;
	int32			fLast;
	int32			fOriginalValue; // The value the user set.
	int32			fValue;			// The value we inferred (addConstraints()).
									// Might be more restrictive or lax.

	QcLinPoly*		fPolynom;
	QcConstraint*	fQocaConstraint;

	bool			fAdded;

private:
	int32			fAlgorithmData;
};


// MinConstraint
class ConstraintSolverLayouter::MinConstraint : public Constraint {
public:
	MinConstraint(ConstraintSolverLayouter* layouter, int32 first, int32 last,
		int32 value)
		: Constraint(layouter, first, last, value)
	{
		SetOriginalValue(value);
	}

	virtual Constraint* CloneConstraint(ConstraintSolverLayouter* layouter)
	{
		Constraint* constraint = new MinConstraint(layouter, fFirst, fLast,
			fOriginalValue);

		constraint->SetValue(fValue);
		
		return constraint;
	}
	
	virtual void RestrictOriginalValue(int32 value)
	{
		SetOriginalValue(max_c(fOriginalValue, value));
	}
	
	virtual void UpdateQocaConstraint()
	{
		fQocaConstraint->makeGe(fPolynom, fValue);
	}
	
	virtual int32 ComputeSatisfactionDistance() const
	{
		return fValue - ComputeVariableSum();
	}

//	public String toString() {
//		return "Constraint[sum (x" + fFirst + " ... x" + fLast + ") >= " + fValue + "]";
//	}
};


// MaxConstraint
class ConstraintSolverLayouter::MaxConstraint : public Constraint {
public:
	MaxConstraint(ConstraintSolverLayouter* layouter, int32 first, int32 last,
		int32 value)
		: Constraint(layouter, first, last, value)
	{
		SetOriginalValue(value);
	}

	virtual Constraint* CloneConstraint(ConstraintSolverLayouter* layouter)
	{
		Constraint* constraint = new MaxConstraint(layouter, fFirst, fLast,
			fOriginalValue);

		constraint->SetValue(fValue);
		
		return constraint;
	}
	
	virtual void RestrictOriginalValue(int32 value)
	{
		SetOriginalValue(min_c(fOriginalValue, value));
	}
	
	virtual void UpdateQocaConstraint()
	{
		fQocaConstraint->makeLe(fPolynom, fValue);
	}
	
	virtual int32 ComputeSatisfactionDistance() const
	{
		return ComputeVariableSum() - fValue;
	}

//	public String toString() {
//		return "Constraint[sum (x" + fFirst + " ... x" + fLast + ") <= " + fValue + "]";
//	}
};


// MinMaxEntry
class ConstraintSolverLayouter::MinMaxEntry {
public:
	Constraint*	minConstraint;
	Constraint*	maxConstraint;
	int32		min;
	int32		max;
	int32		requiredMax;

	MinMaxEntry()
		: minConstraint(NULL),
		  maxConstraint(NULL),
		  min(0),
		  max(B_SIZE_UNLIMITED),
		  requiredMax(0)
	{
	}

	void SetMinConstraint(Constraint* minConstraint)
	{
		this->minConstraint = minConstraint;
		if (minConstraint != NULL)
			min = minConstraint->OriginalValue();
	}

	void SetMaxConstraint(Constraint* maxConstraint)
	{
		this->maxConstraint = maxConstraint;
		if (maxConstraint != NULL)
			max = max_c(min, maxConstraint->OriginalValue());
	}
};


// MinMaxMatrix
class ConstraintSolverLayouter::MinMaxMatrix {
public:
	MinMaxMatrix(int size)
		: fSize(size),
		  fEntries(NULL)
	{
		int count = size * (size + 1) / 2;
		fEntries = new MinMaxEntry[count];
	}

	~MinMaxMatrix()
	{
		delete[] fEntries;
	}

	MinMaxEntry& Entry(int32 first, int32 last)
	{
		return fEntries[first * (2 * fSize - first + 1) / 2 + (last - first)];
	}

private:
	int32			fSize;
	MinMaxEntry*	fEntries;
};


// MyLayoutInfo
class ConstraintSolverLayouter::MyLayoutInfo : public LayoutInfo {
public:
	MyLayoutInfo(int32 elementCount)
		: fCount(elementCount)
	{
		fLocations = new int32[elementCount];
		fSizes = new int32[elementCount];
	}

	~MyLayoutInfo()
	{
		delete fLocations;
		delete fSizes;
	}

	virtual float ElementLocation(int32 element)
	{
		if (element < 0 || element >= fCount)
			return 0;

		return fLocations[element];
	}

	virtual float ElementSize(int32 element)
	{
		if (element < 0 || element >= fCount)
			return -1;

		return fSizes[element] - 1;
	}

	void Dump()
	{
		printf("ConstraintSolverLayouter::MyLayoutInfo(): %ld elements:\n",
			fCount);
		for (int32 i = 0; i < fCount; i++) {
			printf("  %2ld: location: %4ld, size: %4ld\n", i, fLocations[i],
				fSizes[i]);
		}
	}

public:
	int32	fCount;
	int32*	fLocations;
	int32*	fSizes;
};


// #pragma mark - ConstraintSolverLayouter


// constructor
ConstraintSolverLayouter::ConstraintSolverLayouter(int32 elementCount,
	int32 spacing)
	: fElementCount(elementCount),
	  fWeights(NULL),
	  fBasicMinConstraints(NULL),
	  fBasicMaxConstraints(NULL),
	  fComplexMinConstraints(),
	  fComplexMaxConstraints(),
	  fVariables(NULL),
	  fSizeVariable(NULL),
	  fSolver(NULL),
	  fSizeSumConstraint(NULL),
	  fSetSizeConstraint(NULL),
	  fConstraintsAdded(false),
	  fSetSizeConstraintAdded(false),
	  fSpacing(spacing),
	  fMin(0),
	  fMax(B_SIZE_UNLIMITED),
	  fMinMaxValid(false),
	  fLayoutInfo(NULL)
{
	fWeights = new float[fElementCount];
	for (int32 i = 0; i < fElementCount; i++)
		fWeights[i] = 1;
	
	fBasicMinConstraints = new Constraint*[fElementCount];
	fBasicMaxConstraints = new Constraint*[fElementCount];

	memset(fBasicMinConstraints, 0, sizeof(Constraint*) * fElementCount);
	memset(fBasicMaxConstraints, 0, sizeof(Constraint*) * fElementCount);

	fSolver = new QcLinInEqSolver();
	
	fVariables = new Variable[fElementCount];
	for (int i = 0; i < fElementCount; i++) {
		fVariables[i].Init(i);
		fSolver->AddVar(*fVariables[i].QocaVariable());
	}

	fSizeVariable = new Variable("size");
	fSolver->AddVar(*fSizeVariable->QocaVariable());

	// create size sum constraint (x1 + x2 + ... xn == size)
	QcLinPoly* polynom = new QcLinPoly();
	for (int i = 0; i < fElementCount; i++)
		polynom->Push(1, *fVariables[i].QocaVariable());
	polynom->Push(-1, *fSizeVariable->QocaVariable());

	fSizeSumConstraint = new QcConstraint();
	fSizeSumConstraint->makeEq(polynom, 0);

	// create set size constraint (size == <value>)
	polynom = new QcLinPoly();
	polynom->Push(1, *fSizeVariable->QocaVariable());

	fSetSizeConstraint = new QcConstraint();
	fSetSizeConstraint->makeEq(polynom, 0);
}

// destructor
ConstraintSolverLayouter::~ConstraintSolverLayouter()
{
	delete fSetSizeConstraint;
	delete fSizeSumConstraint;
	delete fSizeVariable;
	delete[] fVariables;
	delete fSolver;

	delete[] fWeights;
	delete[] fBasicMinConstraints;
	delete[] fBasicMaxConstraints;
}

// AddConstraints
void
ConstraintSolverLayouter::AddConstraints(int32 element, int32 length,
	float _min, float _max, float _preferred)
{
	if (length <= 0 || length > fElementCount)
		return;
	if (element < 0 || element + length > fElementCount)
		return;

	int32 min = (int32)_min + 1;
	int32 max = (int32)_max + 1;
//	int32 preferred = (int32)_preferred + 1;

	int32 last = element + length - 1;
	int32 spacing = (length - 1) * fSpacing;
	_AddMinConstraint(element, last, min - spacing);
	_AddMaxConstraint(element, last, max - spacing);

	fMinMaxValid = false;
}

// SetWeight
void
ConstraintSolverLayouter::SetWeight(int32 element, float weight)
{
	if (element < 0 || element >= fElementCount)
		return;

	fWeights[element] = max_c(weight, 0);
}

// MinSize
float
ConstraintSolverLayouter::MinSize()
{
	_ValidateMinMax();
	return fMin - 1;
}

// MaxSize
float
ConstraintSolverLayouter::MaxSize()
{
	_ValidateMinMax();
	return fMax - 1;
}

// PreferredSize
float
ConstraintSolverLayouter::PreferredSize()
{
	_ValidateMinMax();
	return fMin - 1;
}

// CreateLayoutInfo
LayoutInfo*
ConstraintSolverLayouter::CreateLayoutInfo()
{
	return new MyLayoutInfo(fElementCount);
}

// Layout
void
ConstraintSolverLayouter::Layout(LayoutInfo* layoutInfo, float _size)
{
//printf("ConstraintSolverLayouter::Layout(%p, %.1f): min: %ld, max: %ld\n",
//layoutInfo, _size, fMin, fMax);
	int32 size = (int32)_size + 1;

	fLayoutInfo = (MyLayoutInfo*)layoutInfo;
	
	_ValidateMinMax();

	if (fElementCount == 0)
		return;

	if (size < fMin)
		size = fMin;

	int32 additionalSpace = max_c(size - fMax, 0);
	size -= additionalSpace;
	int32 spacing = (fElementCount - 1) * fSpacing;
	if (!_ComputeSolution(size - spacing)) {
		ERROR_MESSAGE("ConstraintSolverLayouter::Layout(): no solution\n");
		// no solution
		return;
	}

	// If there is additional space, distribute it according to the elements'
	// weights.
	if (additionalSpace > 0) {
// Mmh, distributing according to the weights doesn't look that good.
//		int[] sizes = new int[fElementCount];
//		SimpleLayouter.distributeSize(additionalSpace, fWeights, sizes);
//			
//		for (int i = 0; i < fElementCount; i++) {
//			Variable var = fVariables[i];
//			var.setIntValue(var.getIntValue() + sizes[i]);
//		}

		// distribute the additional space equally
		int64 sumSize = 0;
		for (int i = 0; i < fElementCount; i++) {
			Variable& var = fVariables[i];
			int64 oldSumSize = sumSize;
			sumSize = (i + 1) * additionalSpace / fElementCount;
			var.SetIntValue(var.IntValue() + (int32)(sumSize - oldSumSize));
		}
	}

	// compute locations
	int location = 0;
	for (int i = 0; i < fElementCount; i++) {
		fLayoutInfo->fLocations[i] = location;
		fLayoutInfo->fSizes[i] = fVariables[i].IntValue();
		location += fSpacing + fLayoutInfo->fSizes[i];
	}

//fLayoutInfo->Dump();
}

// CloneLayouter
Layouter*
ConstraintSolverLayouter::CloneLayouter()
{
	ConstraintSolverLayouter* layouter
		= new ConstraintSolverLayouter(fElementCount, fSpacing);

	memcpy(layouter->fWeights, fWeights, sizeof(float) * fElementCount);

	for (int i = 0; i < fElementCount; i++) {
		Constraint* constraint = fBasicMinConstraints[i];
		if (constraint != NULL) {
			layouter->fBasicMinConstraints[i]
				= constraint->CloneConstraint(layouter);
		}

		constraint = fBasicMaxConstraints[i];
		if (constraint != NULL) {
			layouter->fBasicMaxConstraints[i]
				= constraint->CloneConstraint(layouter);
		}
	}

	int32 count = fComplexMinConstraints.CountItems();
	for (int32 i = 0; i < count; i++) {
		Constraint* constraint = (Constraint*)fComplexMinConstraints.ItemAt(i);
		layouter->fComplexMinConstraints.AddItem(
			constraint->CloneConstraint(layouter));
	}
	
	count = fComplexMaxConstraints.CountItems();
	for (int32 i = 0; i < count; i++) {
		Constraint* constraint = (Constraint*)fComplexMaxConstraints.ItemAt(i);
		layouter->fComplexMaxConstraints.AddItem(
			constraint->CloneConstraint(layouter));
	}

	return layouter;
}

// _ValidateMinMax
void
ConstraintSolverLayouter::_ValidateMinMax()
{
	if (fMinMaxValid)
		return;

	if (fElementCount == 0) {
		fMin = 0;
		fMax = B_SIZE_UNLIMITED;
	} else {
		int32 spacing = (fElementCount - 1) * fSpacing;
		fMin = BLayoutUtils::AddSizesInt32(_ComputeMin(), spacing);
		fMax = BLayoutUtils::AddSizesInt32(_ComputeMax(), spacing);
	}

	fMinMaxValid = true;
}

// _AddMinConstraint
void
ConstraintSolverLayouter::_AddMinConstraint(int32 first, int32 last,
	int32 value)
{
	if (first == last) {
		_SetConstraint(MIN_CONSTRAINT, fBasicMinConstraints, first, value,
			true);
	} else {
		_SetConstraint(MIN_CONSTRAINT, fComplexMinConstraints, first, last,
			value, true);
	}
}

// _AddMaxConstraint
void
ConstraintSolverLayouter::_AddMaxConstraint(int32 first, int32 last,
	int32 value)
{
	if (first == last) {
		_SetConstraint(MAX_CONSTRAINT, fBasicMaxConstraints, first, value,
			true);
 	} else {
		_SetConstraint(MAX_CONSTRAINT, fComplexMaxConstraints, first, last,
			value, true);
 	}
}

// _ComputeMin
int32
ConstraintSolverLayouter::_ComputeMin()
{
	_RemoveSetSizeConstraint();
	_AddConstraints();

	// suggest 0 values for the variables
	for (int32 i = 0; i < fElementCount; i++)
		fSolver->SuggestValue(*fVariables[i].QocaVariable(), 0);
	fSolver->SuggestValue(*fSizeVariable->QocaVariable(), 0);

	// solve
	fSolver->Solve();

	int32 size = lround(fSizeVariable->SolvedValue());

	bool success = _FindIntegerSolution(size);
	if (!success) {
		ERROR_MESSAGE(
			"ConstraintSolverLayouter::_ComputeMin(): no int solution\n");
		return 0;
	}

	return size;
}

// _ComputeMax
int32
ConstraintSolverLayouter::_ComputeMax()
{
	_RemoveSetSizeConstraint();
	_AddConstraints();

	// Try adding the size constraint with max possible value. If that works
	// out, there's no limit on the maximal size.
	if (_AddSetSizeConstraint(B_SIZE_UNLIMITED))
		return B_SIZE_UNLIMITED;
	
	// suggest 0 values for the element variables and the maximal possible
	// value for the size
	for (int32 i = 0; i < fElementCount; i++)
		fSolver->SuggestValue(*fVariables[i].QocaVariable(), 0);
	fSolver->SuggestValue(*fSizeVariable->QocaVariable(), B_SIZE_UNLIMITED);

	// solve
	fSolver->Solve();

	int32 size = lround(fSizeVariable->SolvedValue());
	
	bool success = _FindIntegerSolution(size);
	if (!success) {
		ERROR_MESSAGE(
			"ConstraintSolverLayouter::_ComputeMax(): no int solution\n");
		return B_SIZE_UNLIMITED;
	}

	return size;
}

// _ComputeSolution
bool
ConstraintSolverLayouter::_ComputeSolution(int32 size)
{
//printf("ConstraintSolverLayouter::_ComputeSolution(%ld)\n", size);
	_RemoveSetSizeConstraint();
	_AddConstraints();

	// suggest weighted values for the element variables
	int32 sizes[fElementCount];
	SimpleLayouter::DistributeSize(size, fWeights, sizes, fElementCount);
	for (int32 i = 0; i < fElementCount; i++)
		fSolver->SuggestValue(*fVariables[i].QocaVariable(), sizes[i]);

	fSolver->SuggestValue(*fSizeVariable->QocaVariable(), size);

	if (!_AddSetSizeConstraint(size)) {
		// Ugh! Contradictory!
		ERROR_MESSAGE("ConstraintSolverLayouter::_ComputeSolution(): "
			"contradictory CSP\n");
		return false;
	}

	// solve
	fSolver->Solve();

	if (!_FindIntegerSolution(size)) {
		ERROR_MESSAGE("ConstraintSolverLayouter::_ComputeSolution(): "
			"no int solution\n");
		return false;
	}

	return true;
}

// _FindIntegerSolution
bool
ConstraintSolverLayouter::_FindIntegerSolution(int32 size)
{
//printf("ConstraintSolverLayouter::_FindIntegerSolution(%ld)\n", size);
//printf("  solution:\n");
	// check whether there are non-integer variable values
	Set nonIntVariables;
	int32 sum = 0;
	for (int32 i = 0; i < fElementCount; i++) {
		Variable& var = fVariables[i];
		double value = var.SolvedValue();
		int32 intValue = (int32)floor(value);
//printf("    %ld: %4.2f: %4ld\n", i, value, intValue);

		bool nonIntVar = false;
		if (intValue != value) {
			// the solved value is not integral
			nonIntVar = true;
			
			// make sure the minimum constraint for the variable is satisfied
			// -- if not, this is just a rounding error
			if (fBasicMinConstraints[i] != NULL) {
				if (intValue < fBasicMinConstraints[i]->OriginalValue()) {
					intValue = fBasicMinConstraints[i]->OriginalValue();
					nonIntVar = false;
				}
			}

			// make sure the maximum constraint for the variable is satisfied
			// -- if not, this is just a rounding error
			if (nonIntVar && fBasicMaxConstraints[i] != NULL) {
				// also include the == case here, since the variable can't grow anymore
				if (intValue >= fBasicMaxConstraints[i]->OriginalValue()) {
					intValue = fBasicMaxConstraints[i]->OriginalValue();
					nonIntVar = false;
				}
			}
		}

		var.SetIntValue(intValue);
		sum += intValue;
		
		if (nonIntVar)
			nonIntVariables.Add(&var);
	}

	if (sum == size)
		return true;
	
	if (sum > size) {
		ERROR_MESSAGE("ConstraintSolverLayouter::_FindIntegerSolution(): "
			"computed size %ld, but element sum is %ld\n", size, sum);
		return false;
	}

	if (nonIntVariables.IsEmpty()) {
		ERROR_MESSAGE("ConstraintSolverLayouter::_FindIntegerSolution(): "
			"computed size %ld, element sum is %ld, but we have no "
			"non-integer variables\n", size, sum);
		return false;
	}

	// Apparently rounding down the non-integer variables caused their sum to be less than the computed
	// size. This should cause at least one complex minimum constraint to be violated. We need to
	// distribute the size difference between the variables we have.

	// get the max constraints
	BList maxConstraints;
	int32 count = nonIntVariables.Size();
	for (int32 i = 0; i < count; i++) {
		Variable* var = (Variable*)nonIntVariables.Get(i);
		Constraint* constraint = fBasicMaxConstraints[var->Index()];
		if (constraint != NULL)
			maxConstraints.AddItem(constraint);
	}
	maxConstraints.AddList(&fComplexMaxConstraints);

	return _FindIntegerSolution(nonIntVariables, fComplexMinConstraints,
		maxConstraints, size - sum);
}

// _FindIntegerSolution
bool
ConstraintSolverLayouter::_FindIntegerSolution(const Set& _nonIntVariables,
	const BList& _minConstraints, const BList& _maxConstraints, int32 sizeDiff)
{
	// We get the variables that had a non-integer value, and lists of minimum
	// and maximum constraints that need to be satisfied. The minimum
	// constraints are only those that may be violated at the moment. Maximum
	// constraints are those that may be involved with the variables. First of
	// all we narrow the collections down to their minimal size, i.e. we filter
	// the maximum constraints that are really involved, filter the variables
	// that may still grow (i.e. remove those that are used in barely satisfied
	// maximum constraints), and filter the minimum constraints that are
	// violated.
	// The recursion will end at the latest at sizeDiff levels.
	
	// get the involved maximum constraints
	BList maxConstraints;
	_GetInvolvedConstraints(_maxConstraints, _nonIntVariables.ToList(),
		maxConstraints);

	// Filter the variables according to the maximum constraints that are
	// barely satisfied. Neither of the variables involved can grow anymore.
	Set nonIntVariables;
	_FilterVariablesByMaxConstraints(_nonIntVariables.ToList(), maxConstraints,
		nonIntVariables);
	
	// get the violated minimum constraints; maximum constraints never get
	// violated, since we filter out variables early that, when increase,
	// would cause a maximum constraint to be violated
	BList minConstraints;
	_GetViolatedConstraints(_minConstraints, minConstraints);

	// If the size is correct, we can stop. This is a solution if there are no
	// more violated constraints.
	if (sizeDiff == 0)
		return (minConstraints.IsEmpty());

	// There's at least one violated constraint. If there are no more variables
	// we can temper with, this is apparently a dead end.
	if (nonIntVariables.IsEmpty())
		return false;
	
	// If there is at least one violated constraint, we take the first
	// constraint (the one with the greatest satisfaction distance) and pick
	// the involved variable with the greatest distance to its non-integer
	// value. Increment the variable value, remove the variable, recompute
	// nonIntVariables, minConstraints, maxConstraints, violatedConstraints.
	// If there are no unsatisfied constraints, we simply consider all
	// variables.

	BList constraintVariables;

	if (minConstraints.IsEmpty()) {
		constraintVariables.AddList(&nonIntVariables.ToList());
	} else {
		// get the variables involved in the constraint with the greatest
		// satisfaction distance
		Constraint* constraint = (Constraint*)minConstraints.ItemAt(0);
		constraintVariables.MakeEmpty();
		for (int32 i = constraint->First(); i <= constraint->Last(); i++) {
			Variable* var = &fVariables[i];
			if (nonIntVariables.Contains(var))
				constraintVariables.AddItem(var);
		}
	}

	// sort the variables by real value - integer value difference in
	// descending order
	if (constraintVariables.CountItems() > 1)
		constraintVariables.SortItems(_CompareVariables);

	// Now pick a variable (starting with the one with the greatest
	// real value - integer value difference), increment its integer value
	// and recurse.
	int32 count = constraintVariables.CountItems();
	for (int32 i = 0; i < count; i++) {
		Variable* var = (Variable*)constraintVariables.ItemAt(i);
		int32 varValue = var->IntValue();
		var->SetIntValue(varValue + 1);
		
		// recursion...
		Set remainingVariables;
		remainingVariables.AddAll(nonIntVariables);
		remainingVariables.Remove(var);

		bool success = _FindIntegerSolution(remainingVariables, minConstraints,
			maxConstraints, sizeDiff - 1);
		if (success)
			return true;

		// no solution: reset to old var value
		var->SetIntValue(varValue);
	}

	// if we are here, we didn't find a solution
	ERROR_MESSAGE("ConstraintSolverLayouter::_FindIntegerSolution(): "
		"found no int solution\n");
	return false;
}

// _CreateConstraint
ConstraintSolverLayouter::Constraint*
ConstraintSolverLayouter::_CreateConstraint(int32 type, int32 first,
	int32 last, int32 value)
{
	switch (type) {
		case MIN_CONSTRAINT:
			return new MinConstraint(this, first, last, value);
		case MAX_CONSTRAINT:
			return new MaxConstraint(this, first, last, value);
	}

	// illegal constraint type
	return NULL;
}

// _SetConstraint
void
ConstraintSolverLayouter::_SetConstraint(int32 type, Constraint** constraints,
	int32 element, int32 value, bool restrict)
{
	if (element < 0 || element >= fElementCount)
		return;

	// ignore non-restricting min/max constraints
	if (type == MIN_CONSTRAINT && value <= 0
		|| type == MAX_CONSTRAINT && value >= B_SIZE_UNLIMITED) {
		return;
	}
	
	_RemoveConstraints();
	
	if (constraints[element] != NULL) {
		if (restrict)
			constraints[element]->RestrictOriginalValue(value);
		else
			constraints[element]->SetOriginalValue(value);
	} else
		constraints[element] = _CreateConstraint(type, element, element, value);
}

// _SetConstraint
void
ConstraintSolverLayouter::_SetConstraint(int32 type, BList& constraints,
	int32 first, int32 last, int32 value, bool restrict)
{
	if (first < 0 || first > last || last >= fElementCount)
		return;

	// ignore non-restricting min/max constraints
	if (type == MIN_CONSTRAINT && value <= 0
		|| type == MAX_CONSTRAINT && value >= B_SIZE_UNLIMITED) {
		return;
	}
	
	_RemoveConstraints();
	
	int32 index = _IndexOfConstraint(constraints, first, last);
	if (index >= 0) {
		Constraint* constraint = (Constraint*)constraints.ItemAt(index);
		if (restrict)
			constraint->RestrictOriginalValue(value);
		else
			constraint->SetOriginalValue(value);
	} else {
		Constraint* constraint = _CreateConstraint(type, first, last, value);
		constraints.AddItem(constraint);
	}
}

// _IndexOfConstraint
int
ConstraintSolverLayouter::_IndexOfConstraint(BList& constraints, int32 first,
	int32 last)
{
	int32 count = constraints.CountItems();
	for (int32 i = 0; i < count; i++) {
		Constraint* constraint = (Constraint*)constraints.ItemAt(i);
		if (constraint->First() == first && constraint->Last() == last)
			return i;
	}

	return -1;
}

// _AddSetSizeConstraint
bool
ConstraintSolverLayouter::_AddSetSizeConstraint(int32 size)
{
	if (fSetSizeConstraintAdded) {
		if (fSetSizeConstraint->RHS() == size)
			return true;
		_RemoveSetSizeConstraint();
	}

	fSetSizeConstraint->SetRHS(size);

	bool success = fSolver->AddConstraint(*fSetSizeConstraint);
	if (!success)
		return false;
	
	fSetSizeConstraintAdded = true;
	return true;
}

// _AddConstraints
void
ConstraintSolverLayouter::_AddConstraints()
{
	if (fConstraintsAdded)
		return;
	
	// We try to make constraints non-contradictory (i.e. adjust MAX
	// constraints respectively). That's non-trivial. Example:
	// 0 <= x1 <= inf, 0 <= x2 <= 9, 0 <= x3 <= inf
	// 10 <= x1 + x2 <= inf, 10 <= x2 + x3 <= inf
	// The resulting minimum for x1 + x2 + x3 is not 10, but 11. Naturally a
	// constraint x1 + x2 + x3 <= 10 would be contradictory, but we can't
	// easily infer that. Even if we could recognize the constraints involved
	// in the conflict (the two min and the two max constraints) and decide
	// that we don't want to change min constraints, we still have to decide
	// whether we want adjust the x2 or the x1 + x2 + x3 max constraint.
	//
	// So what we do is, we enforce some of the simpler required properties,
	// like:
	// 1) min_k,j >= min_k,l + min_l,j
	// 2) max_k,j <= max_k,l + max_l,j
	// 3) min_k,j <= max_k,j
	// 4) min_k,j <= max_k,l + max_l,j
	//
	// By adding the constraints in the order min constraints, basic max
	// constraints, complex max constraints we make sure that we only drop
	// complex max constraints, if the CSP is still contradictory.

	// create a MinMaxMatrix we can work with
	MinMaxMatrix matrix(fElementCount);

	// set the basic constraints
	for (int i = 0; i < fElementCount; i++) {
		MinMaxEntry& entry = matrix.Entry(i, i);

		Constraint* minConstraint = fBasicMinConstraints[i];
		if (minConstraint != NULL)
			entry.SetMinConstraint(minConstraint);

		Constraint* maxConstraint = fBasicMaxConstraints[i];
		if (maxConstraint != NULL)
			entry.SetMaxConstraint(maxConstraint);
	}

	// set the complex min constraints
	int32 count = fComplexMinConstraints.CountItems();
	for (int32 i = 0; i < count; i++) {
		Constraint* constraint = (Constraint*)fComplexMinConstraints.ItemAt(i);
		MinMaxEntry& entry = matrix.Entry(constraint->First(),
			constraint->Last());
		entry.SetMinConstraint(constraint);
	}
	
	// set the complex max constraints
	count = fComplexMaxConstraints.CountItems();
	for (int32 i = 0; i < count; i++) {
		Constraint* constraint = (Constraint*)fComplexMaxConstraints.ItemAt(i);
		MinMaxEntry& entry = matrix.Entry(constraint->First(),
			constraint->Last());
		entry.SetMaxConstraint(constraint);
	}

	// create lists containing the constraints (actually only the length of
	// the constraints) starting at a common point
	BList startLists[fElementCount];
	void* one = (void*)1L;
	for (int i = 0; i < fElementCount; i++) {
		startLists[i].AddItem(one);

		MinMaxEntry& entry = matrix.Entry(i, i);
		entry.requiredMax = entry.min;
	}
	
	// make sure that min_k,j >= min_k,l + min_l,j and
	// max_k,j <= max_k,l + max_l,j and min_k,j <= max_k,j
	for (int32 length = 2; length <= fElementCount; length++) {
		for (int32 first = 0; first < fElementCount - length + 1; first++) {
			int32 last = first + length - 1;
			MinMaxEntry& entry = matrix.Entry(first, last);

			count = startLists[first].CountItems();
			for (int32 i = 0; i < count; i++) {
				int32 split = first + (int32)startLists[first].ItemAt(i);
				MinMaxEntry& left = matrix.Entry(first, split - 1);
				MinMaxEntry& right = matrix.Entry(split, last);

				int32 min = BLayoutUtils::AddSizesInt32(left.min, right.min);
				if (min >= entry.min) {
					entry.min = min;
					entry.minConstraint = NULL; // constraint is redundant
				}

				int32 max = BLayoutUtils::AddSizesInt32(left.max, right.max);
				if (max <= entry.max) {
					entry.max = max;
					// TODO: When we relax the max constraints later, the
					// constraint might become a real restriction again. So we
					// don't remove it. We could re-check after relaxing the
					// max constraints.
//					entry.maxConstraint = NULL; // constraint is redundant
				}
			}

			entry.max = max_c(entry.max, entry.min);
			entry.requiredMax = entry.min;
			
			if (entry.minConstraint != NULL || entry.maxConstraint != NULL)
				startLists[first].AddItem((void*)length);
		}
	}

	// now propagate down the requirements on the maximum constraints the min
	// constraints impose, i.e. min_k,j <= max_k,l + max_l,j
	for (int32 length = fElementCount; length >= 2; length--) {
		for (int32 first = 0; first < fElementCount - length + 1; first++) {
			int32 last = first + length - 1;
			MinMaxEntry& entry = matrix.Entry(first, last);

			if (entry.requiredMax > entry.max)
				entry.max = entry.requiredMax;

			count = startLists[first].CountItems();
			for (int32 i = 0; i < count; i++) {
				int leftLen = (int32)startLists[first].ItemAt(i);
				if (leftLen >= length)
					break;

				int32 split = first + leftLen;
				MinMaxEntry& left = matrix.Entry(first, split - 1);
				MinMaxEntry& right = matrix.Entry(split, last);

				int32 max = BLayoutUtils::AddSizesInt32(left.max, right.max);
				if (max < entry.requiredMax) {
					// distribute the difference
					int32 diff = entry.requiredMax - max;
					int32 leftDiff = diff * leftLen / length;
					int32 rightDiff = diff - leftDiff;
					left.requiredMax = max_c(left.requiredMax,
						left.max + leftDiff);
					right.requiredMax = max_c(right.requiredMax,
						right.max + rightDiff);
				}
			}
		}
	}

	// apply the maximum requirements to the basic elements
	for (int32 i = 0; i < fElementCount; i++) {
		MinMaxEntry& entry = matrix.Entry(i, i);
		if (entry.requiredMax > entry.max)
			entry.max = entry.requiredMax;
	}
	
	// now filter and adjust the min/max constraints
	// basic min/max constraints
	BList minConstraints;
	BList maxConstraints;
	for (int32 i = 0; i < fElementCount; i++) {
		MinMaxEntry& entry = matrix.Entry(i, i);
		if (entry.minConstraint != NULL) {
			entry.minConstraint->SetValue(entry.min);
			minConstraints.AddItem(entry.minConstraint);
		}

		if (entry.maxConstraint != NULL) {
			entry.maxConstraint->SetValue(entry.max);
			maxConstraints.AddItem(entry.maxConstraint);
		}
	}
	
	// complex min constraints
	count = fComplexMinConstraints.CountItems();
	for (int32 i = 0; i < count; i++) {
		Constraint* constraint = (Constraint*)fComplexMinConstraints.ItemAt(i);
		MinMaxEntry& entry = matrix.Entry(constraint->First(),
			constraint->Last());
		if (entry.minConstraint != NULL) {
			entry.minConstraint->SetValue(entry.min);
			minConstraints.AddItem(entry.minConstraint);
		}
	}
	
	// complex max constraints
	count = fComplexMaxConstraints.CountItems();
	for (int32 i = 0; i < count; i++) {
		Constraint* constraint = (Constraint*)fComplexMaxConstraints.ItemAt(i);
		MinMaxEntry& entry = matrix.Entry(constraint->First(),
			constraint->Last());
		if (entry.maxConstraint != NULL) {
			entry.maxConstraint->SetValue(entry.max);
			maxConstraints.AddItem(entry.maxConstraint);
		}
	}

	// We first add the min constraints, since they are vital. They can't
	// contradict each other because they only define lower bounds for the
	// variables.
	
	_AddConstraints(minConstraints);

	// Adding a max constraint can fail. Not for the basic constraints though.
	// We ensured that by propagating the consequences of the min constraints
	// down. Complex max constraints may conflict with a combination of other
	// max and some min constraints, though. Hence we sort the constraints by
	// length (i.e. number of involved elements).

	maxConstraints.SortItems(_CompareConstraintsByLength);
	_AddConstraints(maxConstraints);
	
	// finally add the size sum constraint (which isn't a constraint really)
	if (!fSolver->AddConstraint(*fSizeSumConstraint)) {
		// That's something that can never possibly happen, since this
		// constraint just introduces the size variable, defining it to be the
		// sum of all elements.
// 		System.out.println("Failed to add sum constraint!");
	}

	fConstraintsAdded = true;
}

// _AddConstraints
void
ConstraintSolverLayouter::_AddConstraints(const BList& constraints)
{
	int32 count = constraints.CountItems();
	for (int32 i = 0; i < count; i++) {
		Constraint* constraint = (Constraint*)constraints.ItemAt(i);
		bool success = fSolver->AddConstraint(*constraint->QocaConstraint());
		if (success) {
			constraint->SetAdded(true);
		} else {
			// Failed to add the constraint: This can only happen for max
			// constraints, when the other _AddConstraints() method wasn't able
			// to resolve the conflict. We try to relax the constraint until
			// there no longer is a conflict.
			// The alternative would be to drop it completely, which might even
			// cause that there remains no maximum restriction at all, which is
			// certainly undesired.

			// find an upper bound first
			int32 value = constraint->Value();
			int32 lower = value + 1;
			int32 upper = max_c(lower, 128);
			bool foundUpperBound = false;
			while (upper < B_SIZE_UNLIMITED / 2) {
				constraint->SetValue(upper);
				if (fSolver->AddConstraint(*constraint->QocaConstraint())) {
					foundUpperBound = true;
					fSolver->RemoveConstraint(*constraint->QocaConstraint());
					break;
				}

				lower = upper + 1;
				upper *= 2;
			}

			if (!foundUpperBound) {
//				System.out.println("Failed to add constraint: " + constraint);
				continue;
			}

			// now narrow the range down until we found the exact upper bound
			while (lower < upper) {
				int32 mid = (lower + upper) / 2;

				constraint->SetValue(mid);
				if (fSolver->AddConstraint(*constraint->QocaConstraint())) {
					upper = mid;
					fSolver->RemoveConstraint(*constraint->QocaConstraint());
				} else
					lower = mid + 1;
			}

			// now add the constraint for real
			constraint->SetValue(upper);
			if (fSolver->AddConstraint(*constraint->QocaConstraint())) {
//				System.out.println("Added constraint: " + constraint + ". Original value was: " + value);
				constraint->SetAdded(true);
			} else {
//				System.out.println("Failed to add constraint " + constraint + ", although it worked "
//					+ "before!");
			}
		}
	}
}

// _RemoveSetSizeConstraint
void
ConstraintSolverLayouter::_RemoveSetSizeConstraint()
{
	if (fSetSizeConstraintAdded) {
		fSolver->RemoveConstraint(*fSetSizeConstraint);
		fSetSizeConstraintAdded = false;
	}
}

// _RemoveConstraints
void
ConstraintSolverLayouter::_RemoveConstraints()
{
	if (!fConstraintsAdded)
		return;

	// set size constraint
	_RemoveSetSizeConstraint();
	
	// size sum constraint
	fSolver->RemoveConstraint(*fSizeSumConstraint);
	
	// basic min/max constraints
	for (int32 i = 0; i < fElementCount; i++) {
		_RemoveConstraint(fBasicMinConstraints[i]);
		_RemoveConstraint(fBasicMaxConstraints[i]);
	}

	// complex min constraints
	int32 count = fComplexMinConstraints.CountItems();
	for (int32 i = 0; i < count; i++)
		_RemoveConstraint((Constraint*)fComplexMinConstraints.ItemAt(i));

	// complex max constraints
	count = fComplexMaxConstraints.CountItems();
	for (int32 i = 0; i < count; i++)
		_RemoveConstraint((Constraint*)fComplexMaxConstraints.ItemAt(i));
}

// _RemoveConstraint
void
ConstraintSolverLayouter::_RemoveConstraint(Constraint* constraint)
{
	if (constraint != NULL && constraint->IsAdded()) {
		fSolver->RemoveConstraint(*constraint->QocaConstraint());
		constraint->SetAdded(false);
	}
}

// _GetInvolvedConstraints
void
ConstraintSolverLayouter::_GetInvolvedConstraints(const BList& constraints,
	const BList& variables, BList& filteredConstraints)
{
	filteredConstraints.MakeEmpty();
	if (constraints.IsEmpty() || variables.IsEmpty())
		return;

	// create a fast lookup array for the variables
	int32 varDistances[fElementCount];
	int32 count = variables.CountItems();
	for (int32 i = 0; i < count; i++) {
		Variable* var = (Variable*)variables.ItemAt(i);
		varDistances[var->Index()] = 1;
	}

	int32 nextSetVar = fElementCount;
	for (int32 i = fElementCount - 1; i >= 0; i--) {
		if (varDistances[i] == 1)
			nextSetVar = i;
		varDistances[i] = nextSetVar - i;
	}

	// now filter the constraints
	count = constraints.CountItems();
	for (int32 i = 0; i < count; i++) {
		Constraint* constraint = (Constraint*)constraints.ItemAt(i);
		int32 first = constraint->First();
		if (first + varDistances[first] <= constraint->Last())
			filteredConstraints.AddItem(constraint);
	}
}

// _GetViolatedConstraints
void
ConstraintSolverLayouter::_GetViolatedConstraints(const BList& constraints,
	BList& violatedConstraints)
{
	violatedConstraints.MakeEmpty();

	int32 count = constraints.CountItems();
	for (int32 i = 0; i < count; i++) {
		Constraint* constraint = (Constraint*)constraints.ItemAt(i);
		int32 distance = constraint->ComputeSatisfactionDistance();
		if (distance > 0) {
			constraint->SetAlgorithmData(distance);
			violatedConstraints.AddItem(constraint);
		}
	}

	// sort the constraints by (descending) satisfaction distance
	if (violatedConstraints.CountItems() > 1) {
		violatedConstraints.SortItems(
			_CompareConstraintsBySatisfactionDistance);
	}
}

// _FilterVariablesByMaxConstraints
void
ConstraintSolverLayouter::_FilterVariablesByMaxConstraints(
	const BList& variables, const BList& constraints, Set& filteredVars)
{
	filteredVars.MakeEmpty();
	if (variables.IsEmpty())
		return;

	filteredVars.AddAll(variables);
	if (constraints.IsEmpty())
		return;
		
	int32 count = constraints.CountItems();
	for (int32 k = 0; k < count; k++) {
		Constraint* constraint = (Constraint*)constraints.ItemAt(k);
		int32 distance = constraint->ComputeSatisfactionDistance();
		if (distance >= 0) {
			for (int32 i = constraint->First(); i <= constraint->Last(); i++)
				filteredVars.Remove(&fVariables[i]);

			if (filteredVars.IsEmpty())
				break;
		}
	}
}

// _CompareVariables
int
ConstraintSolverLayouter::_CompareVariables(const void* _a, const void* _b)
{
	// compare by distance of the float solution to the integer solution
	Variable* a = *(Variable**)_a;
	Variable* b = *(Variable**)_b;
	double diff1 = a->SolvedValue() - a->IntValue();
	double diff2 = b->SolvedValue() - b->IntValue();

	// descending order
	if (diff2 < diff1)
		return -1;
	return (diff2 > diff1 ? 1 : 0);
}

// _CompareConstraints
int
ConstraintSolverLayouter::_CompareConstraintsByLength(const void* _a,
	const void* _b)
{
	// compare by number of involved variables
	Constraint* a = *(Constraint**)_a;
	Constraint* b = *(Constraint**)_b;

	return (a->Last() - a->First()) - (b->Last() - b->First());
}

// _CompareConstraints
int
ConstraintSolverLayouter::_CompareConstraintsBySatisfactionDistance(
	const void* _a, const void* _b)
{
	// compare by number of involved variables
	Constraint* a = *(Constraint**)_a;
	Constraint* b = *(Constraint**)_b;

	// descending
	return b->AlgorithmData() - a->AlgorithmData();
}
