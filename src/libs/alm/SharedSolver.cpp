/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SharedSolver.h"

#include <ALMLayout.h>
#include <ObjectList.h>


using LinearProgramming::LinearSpec;


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
SharedSolver::LayoutLeaving(BALMLayout* layout)
{
	fLayouts.RemoveItem(layout);
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
	_SetContext(NULL);
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
