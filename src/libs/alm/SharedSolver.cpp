/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SharedSolver.h"

#include <set>

#include <ALMLayout.h>
#include <AutoDeleter.h>
#include <ObjectList.h>

#include "RowColumnManager.h"


using LinearProgramming::LinearSpec;


namespace {

type_code kConstraintsTypeCode = 'cnst';
const char* kConstraintsField = "SharedSolver:constraints";

};


struct SharedSolver::MinSizeValidator {
	inline void CallSolverMethod(LinearSpec* spec, VariableList* vars)
	{
		spec->FindMins(vars);
	}

	inline void Finalize(BALMLayout* layout, SharedSolver* solver,
		ResultType solveResult)
	{
		if (solveResult == LinearProgramming::kUnbounded) {
			solver->SetMinSize(layout, BSize(0, 0));
		} else {
			solver->SetMinSize(layout, BSize(layout->Right()->Value(),
				layout->Bottom()->Value()));
		}
	}
};


struct SharedSolver::MaxSizeValidator {
	inline void CallSolverMethod(LinearSpec* spec, VariableList* vars)
	{
		spec->FindMaxs(vars);
	}

	inline void Finalize(BALMLayout* layout, SharedSolver* solver,
		ResultType solveResult)
	{
		if (solveResult == LinearProgramming::kUnbounded) {
			solver->SetMaxSize(layout,
				BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
		} else {
			solver->SetMaxSize(layout, BSize(layout->Right()->Value(),
				layout->Bottom()->Value()));
		}
	}
};


struct SharedSolver::PreferredSizeValidator {
	inline void CallSolverMethod(LinearSpec* spec, VariableList* vars)
	{
		spec->Solve();
	}

	inline void Finalize(BALMLayout* layout, SharedSolver* solver,
		ResultType solveResult)
	{
		float width = layout->Right()->Value() - layout->Left()->Value();
		float height = layout->Top()->Value() - layout->Bottom()->Value();
		solver->SetPreferredSize(layout, BSize(width, height));
	}
};


SharedSolver::SharedSolver()
	:
	fConstraintsValid(false),
	fMinValid(false),
	fMaxValid(false),
	fPreferredValid(false),
	fLayoutValid(false),
	fLayoutContext(NULL)
{
}


SharedSolver::SharedSolver(BMessage* archive)
	:
	BArchivable(archive),
	fConstraintsValid(false),
	fMinValid(false),
	fMaxValid(false),
	fPreferredValid(false),
	fLayoutValid(false),
	fLayoutContext(NULL)
{
}


SharedSolver::~SharedSolver()
{
	if (fLayouts.CountItems() > 0)
		debugger("SharedSolver deleted while still in use!");

	_SetContext(NULL);
}


LinearSpec*
SharedSolver::Solver() const
{
	return const_cast<LinearSpec*>(&fLinearSpec);
}


void
SharedSolver::Invalidate(bool children)
{
	if (!fConstraintsValid && !fMaxValid && !fMinValid && !fLayoutValid)
		return;

	fConstraintsValid = false;
	fMinValid = false;
	fMaxValid = false;
	fPreferredValid = false;
	fLayoutValid = false;

	_SetContext(NULL);

	for (int32 i = fLayouts.CountItems() - 1; i >= 0; i--)
		fLayouts.ItemAt(i)->InvalidateLayout(children);
}


void
SharedSolver::RegisterLayout(BALMLayout* layout)
{
	fLayouts.AddItem(layout);
	Invalidate(false);
}


void
SharedSolver::LayoutLeaving(const BALMLayout* layout)
{
	fLayouts.RemoveItem(const_cast<BALMLayout*>(layout));
	Invalidate(false);
}


ResultType
SharedSolver::ValidateMinSize()
{
	_Validate<MinSizeValidator>(fMinValid, fMinResult);
	return fMinResult;
}


ResultType
SharedSolver::ValidateMaxSize()
{
	_Validate<MaxSizeValidator>(fMaxValid, fMaxResult);
	return fMaxResult;
}


ResultType
SharedSolver::ValidatePreferredSize()
{
	_Validate<PreferredSizeValidator>(fPreferredValid, fPreferredResult);
	return fPreferredResult;
}


ResultType
SharedSolver::ValidateLayout(BLayoutContext* context)
{
	if (fLayoutValid && fLayoutContext == context)
		return fLayoutResult;

	_SetContext(context);
	_ValidateConstraints();

	for (int32 i = fLayouts.CountItems() - 1; i >= 0; i--) {
		BALMLayout* layout = fLayouts.ItemAt(i);
		BSize size(layout->LayoutArea().Size());
		layout->Right()->SetRange(size.width, size.width);
		layout->Bottom()->SetRange(size.height, size.height);
	}

	fLayoutResult = fLinearSpec.Solve();
	fLayoutValid = true;
	return fLayoutResult;
}


status_t
SharedSolver::Archive(BMessage* archive, bool deep) const
{
	return BArchivable::Archive(archive, deep);
}


status_t
SharedSolver::AllArchived(BMessage* archive) const
{
	/*
	for each archived layout:
		add it to our archive
		add constraints created by areas and column manager to a set
		add range constraints on the left/top/right/bottom tabs to the same set
	for each constraint in the linear spec:
		if it is not in the set above:
			archive it
	*/
	BArchiver archiver(archive);
	std::set<const Constraint*> autoConstraints;
	for (int32 i = fLayouts.CountItems() - 1; i >= 0; i--) {
		BALMLayout* layout = fLayouts.ItemAt(i);
		if (!archiver.IsArchived(layout))
			continue;

		for (int32 j = layout->CountItems() - 1; j >= 0; j--)
			_AddConstraintsToSet(layout->AreaAt(j), autoConstraints);

		for (int32 j = layout->fRowColumnManager->fRows.CountItems() - 1;
				j >= 0; j--) {
			Row* row = layout->fRowColumnManager->fRows.ItemAt(j);
			autoConstraints.insert(row->fPrefSizeConstraint);
		}

		for (int32 j = layout->fRowColumnManager->fColumns.CountItems() - 1;
				j >= 0; j--) {
			Column* column = layout->fRowColumnManager->fColumns.ItemAt(j);
			autoConstraints.insert(column->fPrefSizeConstraint);
		}

		Variable* corners[] = {layout->fLeft, layout->fTop, layout->fRight,
			layout->fBottom};

		for (int32 j = 0; j < 4; j++) {
			const Constraint* min;
			const Constraint* max;
			fLinearSpec.GetRangeConstraints(corners[j], &min, &max);
			if (min)
				autoConstraints.insert(min);
			if (max)
				autoConstraints.insert(max);
		}
	}

	status_t err = B_OK;
	const ConstraintList& constraints = fLinearSpec.Constraints();
	for (int32 i = constraints.CountItems() - 1; i >= 0 && err == B_OK; i--) {
		Constraint* constraint = constraints.ItemAt(i);
		if (autoConstraints.find(constraint) == autoConstraints.end())
			err = _AddConstraintToArchive(constraint, archive);
	}
	return err;
}


status_t
SharedSolver::AllUnarchived(const BMessage* archive)
{
	int32 count = 0;
	archive->GetInfo(kConstraintsField, NULL, &count);

	status_t err = B_OK;
	BUnarchiver unarchiver(archive);
	for (int32 i = 0; i < count && err == B_OK; i++) {
		const void* constraintData;
		ssize_t numBytes;
		err = archive->FindData(kConstraintsField, kConstraintsTypeCode,
			i, &constraintData, &numBytes);
		if (err != B_OK)
			return err;

		err = _InstantiateConstraint(constraintData, numBytes, unarchiver);
	}
	return err;
}


void
SharedSolver::_AddConstraintsToSet(Area* area,
	std::set<const Constraint*>& constraints)
{
	if (area->fMinContentWidth)
		constraints.insert(area->fMinContentWidth);
	if (area->fMaxContentWidth)
		constraints.insert(area->fMaxContentWidth);
	if (area->fMinContentHeight)
		constraints.insert(area->fMinContentHeight);
	if (area->fMaxContentHeight)
		constraints.insert(area->fMaxContentHeight);
	if (area->fContentAspectRatioC)
		constraints.insert(area->fContentAspectRatioC);
}


status_t
SharedSolver::_AddConstraintToArchive(Constraint* constraint,
	BMessage* archive)
{
	/* Format:
	 * int32: summandCount
	 *  { int32 : token
	 *    double: coefficient
	 *  } [summandCount]	
	 *  int32: operator
	 *  double: rightSide
	 *  double: penaltyNeg
	 *  double: penaltyPos
	 */

	// TODO: check Read/Write calls
	BArchiver archiver(archive);
	BMallocIO buffer;
	SummandList* leftSide = constraint->LeftSide();
	int32 summandCount = leftSide->CountItems();
	buffer.Write(&summandCount, sizeof(summandCount));

	for (int32 i = 0; i < summandCount; i++) {
		Summand* summand = leftSide->ItemAt(i);
		Variable* var = summand->Var();
		BArchivable* varArchivable = dynamic_cast<BArchivable*>(var);
		if (!varArchivable || !archiver.IsArchived(varArchivable))
			return B_NAME_NOT_FOUND;

		int32 token;
		archiver.GetTokenForArchivable(varArchivable, token);
		buffer.Write(&token, sizeof(token));

		double coefficient = summand->Coeff();
		buffer.Write(&coefficient, sizeof(coefficient));
	}

	int32 op = constraint->Op();
	buffer.Write(&op, sizeof(op));

	double rightSide = constraint->RightSide();
	buffer.Write(&rightSide, sizeof(rightSide));
	double penaltyNeg = constraint->PenaltyNeg();
	buffer.Write(&penaltyNeg, sizeof(penaltyNeg));
	double penaltyPos = constraint->PenaltyPos();
	buffer.Write(&penaltyPos, sizeof(penaltyPos));

	return archive->AddData(kConstraintsField, kConstraintsTypeCode,
		buffer.Buffer(), buffer.BufferLength(), false);
}


status_t
SharedSolver::_InstantiateConstraint(const void* rawData, ssize_t numBytes,
	BUnarchiver& unarchiver)
{
	/* Format:
	 * int32: summandCount
	 *  { int32 : token
	 *    double: coefficient
	 *  } [summandCount]	
	 *  int32: operator
	 *  double: rightSide
	 *  double: penaltyNeg
	 *  double: penaltyPos
	 */

	// TODO: check Read/Write calls
	BMemoryIO buffer(rawData, numBytes);
	int32 summandCount;
	buffer.Read((void*)&summandCount, sizeof(summandCount));

	SummandList* summandList = new SummandList(20, true);
	ObjectDeleter<SummandList> deleter(summandList);
	status_t err = B_OK;
	for (int32 i = 0; i < summandCount; i++) {
		int32 token;
		buffer.Read((void*)&token, sizeof(token));

		BArchivable* varArchivable;
		err = unarchiver.GetObject(token, varArchivable);
		if (err != B_OK)
			break;
		Variable* var = dynamic_cast<Variable*>(varArchivable);
		if (!var)
			return B_BAD_TYPE;

		fLinearSpec.AddVariable(var);

		double coefficient;
		buffer.Read(&coefficient, sizeof(coefficient));

		summandList->AddItem(new Summand(coefficient, var));
	}

	int32 op;
	buffer.Read(&op, sizeof(op));

	double rightSide;
	buffer.Read(&rightSide, sizeof(rightSide));
	double penaltyNeg;
	buffer.Read(&penaltyNeg, sizeof(penaltyNeg));
	double penaltyPos;
	buffer.Read(&penaltyPos, sizeof(penaltyPos));

	if (err != B_OK)
		return err;

	if (fLinearSpec.AddConstraint(summandList, (OperatorType)op, rightSide,
			penaltyNeg, penaltyPos) != NULL) {
		deleter.Detach();
		return B_OK;
	}

	return B_NO_MEMORY;
}


void
SharedSolver::SetMaxSize(BALMLayout* layout, const BSize& max)
{
	layout->fMaxSize = max;
}


void
SharedSolver::SetMinSize(BALMLayout* layout, const BSize& min)
{
	layout->fMinSize = min;
}


void
SharedSolver::SetPreferredSize(BALMLayout* layout, const BSize& preferred)
{
	layout->fPreferredSize = preferred;
}


void
SharedSolver::LayoutContextLeft(BLayoutContext* context)
{
	fLayoutContext = NULL;
}


void
SharedSolver::_SetContext(BLayoutContext* context)
{
	if (fLayoutContext)
		fLayoutContext->RemoveListener(this);
	fLayoutContext = context;
	if (fLayoutContext)
		fLayoutContext->AddListener(this);
}


void
SharedSolver::_ValidateConstraints()
{
	if (!fConstraintsValid) {
		for (int32 i = fLayouts.CountItems() - 1; i >= 0; i--) {
			fLayouts.ItemAt(i)->UpdateConstraints(fLayoutContext);
		}
		fConstraintsValid = true;
	}
}


template <class Validator>
void
SharedSolver::_Validate(bool& isValid, ResultType& result)
{
	if (isValid)
		return;

	_ValidateConstraints();

	VariableList variables(fLayouts.CountItems() * 2);
	Validator validator;

	for (int32 i = fLayouts.CountItems() - 1; i >= 0; i--) {
		BALMLayout* layout = fLayouts.ItemAt(i);
		layout->Right()->SetRange(0, 20000);
		layout->Bottom()->SetRange(0, 20000);

		variables.AddItem(layout->Right());
		variables.AddItem(layout->Bottom());
	}

	validator.CallSolverMethod(&fLinearSpec, &variables);
	result = fLinearSpec.Result();

	for (int32 i = fLayouts.CountItems() - 1; i >= 0; i--)
		validator.Finalize(fLayouts.ItemAt(i), this, result);

	isValid = true;
	fLayoutValid = false;
}


BArchivable*
SharedSolver::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BPrivate::SharedSolver"))
		return new SharedSolver(archive);
	return NULL;
}
