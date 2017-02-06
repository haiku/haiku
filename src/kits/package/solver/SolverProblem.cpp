/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <Catalog.h>

#include <package/solver/SolverProblem.h>

#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblemSolution.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SolverProblem"


static const char* const kToStringTexts[] = {
	B_TRANSLATE_MARK("unspecified problem"),
	B_TRANSLATE_MARK("%source% does not belong to a distupgrade repository"),
	B_TRANSLATE_MARK("%source% has inferior architecture"),
	B_TRANSLATE_MARK("problem with installed package %source%"),
	B_TRANSLATE_MARK("conflicting requests"),
	B_TRANSLATE_MARK("nothing provides requested %dependency%"),
	B_TRANSLATE_MARK("%dependency% is provided by the system"),
	B_TRANSLATE_MARK("dependency problem"),
	B_TRANSLATE_MARK("package %source% is not installable"),
	B_TRANSLATE_MARK("nothing provides %dependency% needed by %source%"),
	B_TRANSLATE_MARK("cannot install both %source% and %target%"),
	B_TRANSLATE_MARK("package %source% conflicts with %dependency% provided "
		"by %target%"),
	B_TRANSLATE_MARK("package %source% obsoletes %dependency% provided by "
		"%target%"),
	B_TRANSLATE_MARK("installed package %source% obsoletes %dependency% "
		"provided by %target%"),
	B_TRANSLATE_MARK("package %source% implicitly obsoletes %dependency% "
		"provided by %target%"),
	B_TRANSLATE_MARK("package %source% requires %dependency%, but none of the "
		"providers can be installed"),
	B_TRANSLATE_MARK("package %source% conflicts with %dependency% provided by "
		"itself")
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

	return BString(B_TRANSLATE_NOCOLLECT(kToStringTexts[index]))
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
