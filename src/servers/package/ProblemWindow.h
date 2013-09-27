/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PROBLEM_WINDOW_H
#define PROBLEM_WINDOW_H


#include <map>
#include <set>

#include <Window.h>


namespace BPackageKit {
	class BSolver;
	class BSolverPackage;
	class BSolverProblem;
	class BSolverProblemSolution;
	class BSolverProblemSolutionElement;
}

using BPackageKit::BSolver;
using BPackageKit::BSolverPackage;
using BPackageKit::BSolverProblem;
using BPackageKit::BSolverProblemSolution;
using BPackageKit::BSolverProblemSolutionElement;

class BButton;
class BGroupView;
class BRadioButton;


class ProblemWindow : public BWindow {
public:
			typedef std::set<BSolverPackage*> SolverPackageSet;

public:
								ProblemWindow();
	virtual						~ProblemWindow();

			bool				Go(BSolver* solver,
									const SolverPackageSet& packagesAddedByUser,
									const SolverPackageSet&
										packagesRemovedByUser);

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

private:
			struct Solution;

			typedef std::map<BRadioButton*, Solution> SolutionMap;

private:
			void				_ClearProblemsGui();
			void				_AddProblemsGui(BSolver* solver);
			void				_AddProblem(BSolverProblem* problem,
									const rgb_color& backgroundColor);
			BString				_SolutionElementText(
									const BSolverProblemSolutionElement*
										element) const;
			bool				_AnySolutionSelected() const;

private:
			sem_id				fDoneSemaphore;
			bool				fClientWaiting;
			bool				fAccepted;
			BGroupView*			fContainerView;
			BButton*			fCancelButton;
			BButton*			fRetryButton;
			SolutionMap			fSolutions;
			const SolverPackageSet* fPackagesAddedByUser;
			const SolverPackageSet* fPackagesRemovedByUser;
};


#endif	// PROBLEM_WINDOW_H
