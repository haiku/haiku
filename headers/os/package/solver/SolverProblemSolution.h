/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__SOLVER_PROBLEM_SOLUTION_H_
#define _PACKAGE__SOLVER_PROBLEM_SOLUTION_H_


#include <ObjectList.h>
#include <String.h>


namespace BPackageKit {


class BSolverPackage;


class BSolverProblemSolutionElement {
public:
			enum BType {
				B_UNSPECIFIED,
				B_DONT_KEEP,
				B_DONT_INSTALL,
				B_DONT_INSTALL_MOST_RECENT,
				B_DONT_FORBID_INSTALLATION,
				B_DONT_DEINSTALL,
				B_DONT_DEINSTALL_ALL,
				B_DONT_LOCK,
				B_KEEP_INFERIOR_ARCHITECTURE,
				B_KEEP_EXCLUDED,
				B_KEEP_OLD,
				B_INSTALL_INFERIOR_ARCHITECTURE,
				B_INSTALL_EXCLUDED,
				B_INSTALL_OLD,
				B_ALLOW_DOWNGRADE,
				B_ALLOW_NAME_CHANGE,
				B_ALLOW_ARCHITECTURE_CHANGE,
				B_ALLOW_VENDOR_CHANGE,
				B_ALLOW_REPLACEMENT,
				B_ALLOW_DEINSTALLATION
			};

public:
								BSolverProblemSolutionElement(BType type,
									BSolverPackage* sourcePackage,
									BSolverPackage* targetPackage,
									const BString& selection);
								~BSolverProblemSolutionElement();

			BType				Type() const;
			BSolverPackage*		SourcePackage() const;
			BSolverPackage*		TargetPackage() const;
			const BString&		Selection() const;

			BString				ToString() const;

private:
			BType				fType;
			BSolverPackage*		fSourcePackage;
			BSolverPackage*		fTargetPackage;
			BString				fSelection;
};


class BSolverProblemSolution {
public:
			typedef BSolverProblemSolutionElement Element;

public:
								BSolverProblemSolution();
								~BSolverProblemSolution();

			int32				CountElements() const;
			const Element*		ElementAt(int32 index) const;

			bool				AppendElement(const Element& element);

private:
			typedef BObjectList<Element> ElementList;

private:
			ElementList			fElements;
};


}	// namespace BPackageKit


#endif // _PACKAGE__SOLVER_PROBLEM_SOLUTION_H_
