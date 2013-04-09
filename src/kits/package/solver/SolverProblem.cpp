/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/solver/SolverProblem.h>

#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblemSolution.h>


static const char* const kToStringTexts[] = {
	"unspecified problem",
	"%source% does not belong to a distupgrade repository",
	"%source% has inferior architecture",
	"problem with installed package %source%",
	"conflicting requests",
	"nothing provides requested %dependency%",
	"%dependency% is provided by the system",
	"dependency problem",
	"package %source% is not installable",
	"nothing provides %dependency% needed by %source%",
	"cannot install both %source% and %target%",
	"package %source% conflicts with %dependency% provided by %target%",
	"package %source% obsoletes %dependency% provided by %target%",
	"installed package %source% obsoletes %dependency% provided by %target%",
	"package %source% implicitly obsoletes %dependency% provided by %target%",
	"package %source% requires %dependency%, but none of the providers can be "
		"installed",
	"package %source% conflicts with %dependency% provided by itself"
};


namespace BPackageKit {


BSolverProblem::BSolverProblem(BType type, BSolverPackage* sourcePackage,
	BSolverPackage* targetPackage)
	:
	fType(type),
	fSourcePackage(sourcePackage),
	fTargetPackage(targetPackage),
	fDependency(),
	fSolutions(10, true)
{
}


BSolverProblem::BSolverProblem(BType type, BSolverPackage* sourcePackage,
	BSolverPackage* targetPackage,
	const BPackageResolvableExpression& dependency)
	:
	fType(type),
	fSourcePackage(sourcePackage),
	fTargetPackage(targetPackage),
	fDependency(dependency),
	fSolutions(10, true)
{
}


BSolverProblem::~BSolverProblem()
{
}


BSolverProblem::BType
BSolverProblem::Type() const
{
	return fType;
}


BSolverPackage*
BSolverProblem::SourcePackage() const
{
	return fSourcePackage;
}


BSolverPackage*
BSolverProblem::TargetPackage() const
{
	return fTargetPackage;
}


const BPackageResolvableExpression&
BSolverProblem::Dependency() const
{
	return fDependency;
}


int32
BSolverProblem::CountSolutions() const
{
	return fSolutions.CountItems();
}


const BSolverProblemSolution*
BSolverProblem::SolutionAt(int32 index) const
{
	return fSolutions.ItemAt(index);
}


bool
BSolverProblem::AppendSolution(BSolverProblemSolution* solution)
{
	return fSolutions.AddItem(solution);
}


BString
BSolverProblem::ToString() const
{
	size_t index = fType;
	if (index >= sizeof(kToStringTexts) / sizeof(kToStringTexts[0]))
		index = 0;

	return BString(kToStringTexts[index])
		.ReplaceAll("%source%",
			fSourcePackage != NULL 
				? fSourcePackage->VersionedName().String() : "?")
		.ReplaceAll("%target%",
			fTargetPackage != NULL
				? fTargetPackage->VersionedName().String() : "?")
		.ReplaceAll("%dependency%",
			fDependency.InitCheck() == B_OK
				? fDependency.ToString().String() : "?");
}


}	// namespace BPackageKit
