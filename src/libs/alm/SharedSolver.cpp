/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SharedSolver.h"

#include <ALMLayout.h>
#include <ObjectList.h>


using LinearProgramming::LinearSpec;


struct LayoutOperation {
	LayoutOperation(BObjectList<BALMLayout>* layouts)
		:
		fLayouts(layouts),
		fVariables(layouts->CountItems() * 2)
	{
	}

	virtual ~LayoutOperation()
	{
	}

	void Validate(SharedSolver* solver)
	{
		for (int32 i = fLayouts->CountItems() - 1; i >= 0; i--) {
			BALMLayout* layout = fLayouts->ItemAt(i);
			layout->Right()->SetRange(0, 20000);
			layout->Bottom()->SetRange(0, 20000);
	
			fVariables.AddItem(layout->Right());
			fVariables.AddItem(layout->Bottom());
		}

		CallSolverMethod(solver->Solver(), &fVariables);

		for (int32 i = fLayouts->CountItems() - 1; i >= 0; i--)
			Finalize(fLayouts->ItemAt(i), solver);
	}

	virtual	void CallSolverMethod(LinearSpec* spec, VariableList* vars) = 0;
	virtual void Finalize(BALMLayout* layout, SharedSolver* solver) = 0;

private:
	BObjectList<BALMLayout>*	fLayouts;
	VariableList				fVariables;
};


struct SharedSolver::MinSizeValidator : LayoutOperation {
	MinSizeValidator(BObjectList<BALMLayout>* layouts)
		:
		LayoutOperation(layouts)
	{
	}

	void CallSolverMethod(LinearSpec* spec, VariableList* vars)
	{
		spec->FindMins(vars);
	}

	void Finalize(BALMLayout* layout, SharedSolver* solver)
	{
		solver->SetMinSize(layout, BSize(layout->Right()->Value(),
			layout->Bottom()->Value()));
	}
};


struct SharedSolver::MaxSizeValidator : LayoutOperation {
	MaxSizeValidator(BObjectList<BALMLayout>* layouts)
		:
		LayoutOperation(layouts)
	{
	}

	void CallSolverMethod(LinearSpec* spec, VariableList* vars)
	{
		spec->FindMaxs(vars);
	}

	void Finalize(BALMLayout* layout, SharedSolver* solver)
	{
		solver->SetMaxSize(layout, BSize(layout->Right()->Value(),
			layout->Bottom()->Value()));
	}
};


SharedSolver::SharedSolver()
	:
	fConstraintsValid(false),
	fMinValid(false),
	fMaxValid(false),
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
	if (fMinValid)
		return fMinResult;

	_ValidateConstraints();

	MinSizeValidator validator(&fLayouts);
	validator.Validate(this);

	fMinResult = fLinearSpec.Result();

	fMinValid = true;
	fLayoutValid = false;
	return fMinResult;
}


ResultType
SharedSolver::ValidateMaxSize()
{
	if (fMaxValid)
		return fMaxResult;

	_ValidateConstraints();

	MaxSizeValidator validator(&fLayouts);
	validator.Validate(this);

	fMaxResult = fLinearSpec.Result();

	fMaxValid = true;
	fLayoutValid = false;

	return fMaxResult;
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
