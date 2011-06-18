/*
 * Copyright 2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "QPSolverInterface.h"

using namespace std;
using namespace LinearProgramming;


QPSolverInterface::QPSolverInterface(LinearSpec* linearSpec)
	:
	SolverInterface(linearSpec)
{

}


QPSolverInterface::~QPSolverInterface()
{
	/*for (unsigned int i = 0; i < fInEqSlackConstraints.size(); i++) {
		SoftInEqData& data = fInEqSlackConstraints[i];
		_DestroyData(data);
	}*/
}


bool
QPSolverInterface::ConstraintAdded(Constraint* constraint)
{
	if (_IsSoftInequality(constraint) == false)
		return true;

	SoftInEqData data;
	double coeff = -1;
	if (constraint->Op() == kGE)
		coeff = 1;
	data.slack = new Summand(coeff, fLinearSpec->AddVariable());
	data.slack->Var()->SetRange(0, 20000);
	data.minSlackConstraint = fLinearSpec->AddConstraint(1.,
		data.slack->Var(), kEQ, 0., constraint->PenaltyNeg(),
		constraint->PenaltyPos());
	fInEqSlackConstraints.push_back(data);

	SummandList* leftSide = constraint->LeftSide();
	leftSide->AddItem(data.slack);
	constraint->SetLeftSide(leftSide);

	return true;
}


bool
QPSolverInterface::ConstraintRemoved(Constraint* constraint)
{
	if (_IsSoftInequality(constraint) == false)
		return true;

	for (unsigned int i = 0; i < fInEqSlackConstraints.size(); i++) {
		SoftInEqData& data = fInEqSlackConstraints[i];
		if (data.minSlackConstraint != constraint)
			continue;

		SummandList* leftSide = constraint->LeftSide();
		leftSide->RemoveItem(data.slack);
		constraint->SetLeftSide(leftSide);
		_DestroyData(data);

		fInEqSlackConstraints.erase(fInEqSlackConstraints.begin() + i);
		break;
	}
	return true;
}


void
QPSolverInterface::_DestroyData(SoftInEqData& data)
{
	fLinearSpec->RemoveConstraint(data.minSlackConstraint);
	fLinearSpec->RemoveVariable(data.slack->Var());
	delete data.slack;
}


bool
QPSolverInterface::_IsSoftInequality(Constraint* constraint)
{
	if (constraint->PenaltyNeg() <= 0. && constraint->PenaltyPos() <= 0.)
		return false;
	if (constraint->Op() != kEQ)
		return true;
	return false;
}
