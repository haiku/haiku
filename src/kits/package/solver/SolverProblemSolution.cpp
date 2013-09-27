/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/solver/SolverProblemSolution.h>

#include <package/solver/SolverPackage.h>


static const char* const kToStringTexts[] = {
	"do something",
	"do not keep %source% installed",
	"do not install \"%selection%\"",
	"do not install the most recent version of \"%selection%\"",
	"do not forbid installation of %source%",
	"do not deinstall \"%selection%\"",
	"do not deinstall all resolvables \"%selection%\"",
	"do not lock \"%selection%\"",
	"keep %source% despite its inferior architecture",
	"keep %source% from excluded repository",
	"keep old %source%",
	"install %source% despite its inferior architecture",
	"install %source% from excluded repository",
	"install %selection% despite its old version",
	"allow downgrade of %source% to %target%",
	"allow name change of %source% to %target%",
	"allow architecture change of %source% to %target%",
	"allow vendor change from \"%sourceVendor%\" (%source%) to "
		"\"%targetVendor%\" (%target%)",
	"allow replacement of %source% with %target%",
	"allow deinstallation of %source%"
};


namespace BPackageKit {


// #pragma mark - BSolverProblemSolutionElement


BSolverProblemSolutionElement::BSolverProblemSolutionElement(BType type,
	BSolverPackage* sourcePackage, BSolverPackage* targetPackage,
	const BString& selection)
	:
	fType(type),
	fSourcePackage(sourcePackage),
	fTargetPackage(targetPackage),
	fSelection(selection)
{
}


BSolverProblemSolutionElement::~BSolverProblemSolutionElement()
{
}


BSolverProblemSolutionElement::BType
BSolverProblemSolutionElement::Type() const
{
	return fType;
}


BSolverPackage*
BSolverProblemSolutionElement::SourcePackage() const
{
	return fSourcePackage;
}


BSolverPackage*
BSolverProblemSolutionElement::TargetPackage() const
{
	return fTargetPackage;
}


const BString&
BSolverProblemSolutionElement::Selection() const
{
	return fSelection;
}


BString
BSolverProblemSolutionElement::ToString() const
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
		.ReplaceAll("%selection%", fSelection)
		.ReplaceAll("%sourceVendor%",
			fSourcePackage != NULL
				? fSourcePackage->Info().Vendor().String() : "?")
		.ReplaceAll("%targetVendor%",
			fTargetPackage != NULL
				? fTargetPackage->Info().Vendor().String() : "?");
}


// #pragma mark - BSolverProblemSolution


BSolverProblemSolution::BSolverProblemSolution()
	:
	fElements(10, true)
{
}


BSolverProblemSolution::~BSolverProblemSolution()
{
}


int32
BSolverProblemSolution::CountElements() const
{
	return fElements.CountItems();
}


const BSolverProblemSolution::Element*
BSolverProblemSolution::ElementAt(int32 index) const
{
	return fElements.ItemAt(index);
}


bool
BSolverProblemSolution::AppendElement(const Element& element)
{
	Element* newElement = new(std::nothrow) Element(element);
	if (newElement == NULL || !fElements.AddItem(newElement)) {
		delete newElement;
		return false;
	}

	return true;
}


}	// namespace BPackageKit
