/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ProblemWindow.h"

#include <Button.h>
#include <Catalog.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <RadioButton.h>
#include <ScrollView.h>
#include <StringView.h>
#include <package/solver/Solver.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>

#include <AutoLocker.h>
#include <package/manager/Exceptions.h>
#include <ViewPort.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageProblem"

using namespace BPackageKit;

using BPackageKit::BManager::BPrivate::BFatalErrorException;


static const uint32 kRetryMessage = 'rtry';
static const uint32 kUpdateRetryButtonMessage = 'uprt';


struct ProblemWindow::Solution {
	BSolverProblem*					fProblem;
	const BSolverProblemSolution*	fSolution;

	Solution()
		:
		fProblem(NULL),
		fSolution(NULL)
	{
	}

	Solution(BSolverProblem* problem, const BSolverProblemSolution* solution)
		:
		fProblem(problem),
		fSolution(solution)
	{
	}
};


ProblemWindow::ProblemWindow()
	:
	BWindow(BRect(0, 0, 400, 300), B_TRANSLATE_COMMENT("Package problems",
			"Window title"), B_TITLED_WINDOW_LOOK,
		B_MODAL_APP_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_MINIMIZABLE | B_AUTO_UPDATE_SIZE_LIMITS,
		B_ALL_WORKSPACES),
	fDoneSemaphore(-1),
	fClientWaiting(false),
	fAccepted(false),
	fContainerView(NULL),
	fCancelButton(NULL),
	fRetryButton(NULL),
	fSolutions(),
	fPackagesAddedByUser(NULL),
	fPackagesRemovedByUser(NULL)

{
	fDoneSemaphore = create_sem(0, "package problems");
	if (fDoneSemaphore < 0)
		throw std::bad_alloc();

	BStringView* topTextView = NULL;
	BViewPort* viewPort = NULL;

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_SMALL_INSETS)
		.Add(topTextView = new BStringView(NULL, B_TRANSLATE(
				"The following problems have been encountered. Please select "
				"a solution for each:")))
		.Add(new BScrollView(NULL, viewPort = new BViewPort(), 0, false, true))
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fCancelButton = new BButton(B_TRANSLATE("Cancel"),
				new BMessage(B_CANCEL)))
			.Add(fRetryButton = new BButton(B_TRANSLATE("Retry"),
				new BMessage(kRetryMessage)))
		.End();

	topTextView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	viewPort->SetChildView(fContainerView = new BGroupView(B_VERTICAL, 0));

	// set small scroll step (large step will be set by the view port)
	font_height fontHeight;
	topTextView->GetFontHeight(&fontHeight);
	float smallStep = ceilf(fontHeight.ascent + fontHeight.descent);
	viewPort->ScrollBar(B_VERTICAL)->SetSteps(smallStep, smallStep);
}


ProblemWindow::~ProblemWindow()
{
	if (fDoneSemaphore >= 0)
		delete_sem(fDoneSemaphore);
}


bool
ProblemWindow::Go(BSolver* solver, const SolverPackageSet& packagesAddedByUser,
	const SolverPackageSet& packagesRemovedByUser)
{
	AutoLocker<ProblemWindow> locker(this);

	fPackagesAddedByUser = &packagesAddedByUser;
	fPackagesRemovedByUser = &packagesRemovedByUser;

	_ClearProblemsGui();
	_AddProblemsGui(solver);

	fCancelButton->SetEnabled(true);
	fRetryButton->SetEnabled(false);

	if (IsHidden()) {
		CenterOnScreen();
		Show();
	}

	fAccepted = false;
	fClientWaiting = true;

	locker.Unlock();

	while (acquire_sem(fDoneSemaphore) == B_INTERRUPTED) {
	}

	locker.Lock();
	if (!locker.IsLocked() || !fAccepted || !_AnySolutionSelected())
		return false;

	// set the solutions
	for (SolutionMap::const_iterator it = fSolutions.begin();
		it != fSolutions.end(); ++it) {
		BRadioButton* button = it->first;
		if (button->Value() == B_CONTROL_ON) {
			const Solution& solution = it->second;
			status_t error = solver->SelectProblemSolution(solution.fProblem,
				solution.fSolution);
			if (error != B_OK)
				throw BFatalErrorException(error, "failed to set solution");
		}
	}

	return true;
}


bool
ProblemWindow::QuitRequested()
{
	if (fClientWaiting) {
		fClientWaiting = false;
		release_sem(fDoneSemaphore);
	}
	return true;
}


void
ProblemWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_CANCEL:
			Hide();
			fClientWaiting = false;
			release_sem(fDoneSemaphore);
			break;
		case kRetryMessage:
			fCancelButton->SetEnabled(false);
			fRetryButton->SetEnabled(false);
			fAccepted = true;
			fClientWaiting = false;
			release_sem(fDoneSemaphore);
			break;
		case kUpdateRetryButtonMessage:
			fRetryButton->SetEnabled(_AnySolutionSelected());
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ProblemWindow::_ClearProblemsGui()
{
	fSolutions.clear();

	int32 count = fContainerView->CountChildren();
	for (int32 i = count - 1; i >= 0; i--) {
		BView* child = fContainerView->ChildAt(i);
		fContainerView->RemoveChild(child);
		delete child;
	}
}


void
ProblemWindow::_AddProblemsGui(BSolver* solver)
{
	int32 problemCount = solver->CountProblems();
	for (int32 i = 0; i < problemCount; i++) {
		_AddProblem(solver->ProblemAt(i),
			(i & 1) == 0 ? B_NO_TINT : 1.04);
	}
}


void
ProblemWindow::_AddProblem(BSolverProblem* problem,
	const float backgroundTint)
{
	BGroupView* problemGroup = new BGroupView(B_VERTICAL);
	fContainerView->AddChild(problemGroup);
	problemGroup->GroupLayout()->SetInsets(B_USE_SMALL_INSETS);
	problemGroup->SetViewUIColor(B_LIST_BACKGROUND_COLOR, backgroundTint);

	BStringView* problemView = new BStringView(NULL, problem->ToString());
	problemGroup->AddChild(problemView);
	BFont problemFont;
	problemView->GetFont(&problemFont);
	problemFont.SetFace(B_BOLD_FACE);
	problemView->SetFont(&problemFont);
	problemView->AdoptParentColors();

	int32 solutionCount = problem->CountSolutions();
	for (int k = 0; k < solutionCount; k++) {
		const BSolverProblemSolution* solution = problem->SolutionAt(k);
		BRadioButton* solutionButton = new BRadioButton(
			BString().SetToFormat(B_TRANSLATE_COMMENT("solution %d:",
				"Don't change the %d variable"), k + 1),
			new BMessage(kUpdateRetryButtonMessage));
		problemGroup->AddChild(solutionButton);

		BGroupLayout* elementsGroup = new BGroupLayout(B_VERTICAL);
		problemGroup->AddChild(elementsGroup);
		elementsGroup->SetInsets(20, 0, 0, 0);

		int32 elementCount = solution->CountElements();
		for (int32 l = 0; l < elementCount; l++) {
			const BSolverProblemSolutionElement* element
				= solution->ElementAt(l);
			BStringView* elementView = new BStringView(NULL,
				BString().SetToFormat("- %s",
					_SolutionElementText(element).String()));
			elementsGroup->AddView(elementView);
			elementView->AdoptParentColors();
		}

		fSolutions[solutionButton] = Solution(problem, solution);
	}

	BRadioButton* ignoreButton = new BRadioButton(B_TRANSLATE(
		"ignore problem for now"), new BMessage(kUpdateRetryButtonMessage));
	problemGroup->AddChild(ignoreButton);
	ignoreButton->SetValue(B_CONTROL_ON);
}


BString
ProblemWindow::_SolutionElementText(
	const BSolverProblemSolutionElement* element) const
{
	// Reword text for B_ALLOW_DEINSTALLATION, if the package has been added
	// by the user.
	BSolverPackage* package = element->SourcePackage();
	if (element->Type() == BSolverProblemSolutionElement::B_ALLOW_DEINSTALLATION
		&& package != NULL
		&& fPackagesAddedByUser->find(package) != fPackagesAddedByUser->end()) {
		return BString(B_TRANSLATE_COMMENT("don't activate package %source%",
				"don't change '%source%")).ReplaceAll(
				"%source%", package->VersionedName());
	}

	return element->ToString();
}


bool
ProblemWindow::_AnySolutionSelected() const
{
	for (SolutionMap::const_iterator it = fSolutions.begin();
		it != fSolutions.end(); ++it) {
		BRadioButton* button = it->first;
		if (button->Value() == B_CONTROL_ON)
			return true;
	}

	return false;
}
