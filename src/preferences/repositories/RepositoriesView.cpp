/*
 * Copyright 2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill
 */


#include "RepositoriesView.h"

#include <stdlib.h>
#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <ColumnTypes.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <ScrollBar.h>
#include <SeparatorView.h>
#include <Url.h>
#include <package/PackageRoster.h>
#include <package/RepositoryConfig.h>

#include "constants.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RepositoriesView"


static const BString kTitleEnabled =
	B_TRANSLATE_COMMENT("Status", "Column title");
static const BString kTitleName = B_TRANSLATE_COMMENT("Name", "Column title");
static const BString kTitleUrl = B_TRANSLATE_COMMENT("URL", "Column title");
static const BString kLabelRemove =
	B_TRANSLATE_COMMENT("Remove", "Button label");
static const BString kLabelRemoveAll =
	B_TRANSLATE_COMMENT("Remove All", "Button label");
static const BString kLabelEnable =
	B_TRANSLATE_COMMENT("Enable", "Button label");
static const BString kLabelEnableAll =
	B_TRANSLATE_COMMENT("Enable All", "Button label");
static const BString kLabelDisable =
	B_TRANSLATE_COMMENT("Disable", "Button label");
static const BString kLabelDisableAll =
	B_TRANSLATE_COMMENT("Disable All", "Button label");
static const BString kStatusViewText =
	B_TRANSLATE_COMMENT("Changes pending:", "Status view text");
static const BString kStatusCompletedText =
	B_TRANSLATE_COMMENT("Changes completed", "Status view text");


RepositoriesListView::RepositoriesListView(const char* name)
	:
	BColumnListView(name, B_NAVIGABLE, B_PLAIN_BORDER)
{
}


void
RepositoriesListView::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_DELETE:
			Window()->PostMessage(DELETE_KEY_PRESSED);
			break;

		default:
			BColumnListView::KeyDown(bytes, numBytes);
	}
}


RepositoriesView::RepositoriesView()
	:
	BGroupView("RepositoriesView"),
	fTaskLooper(NULL),
	fShowCompletedStatus(false),
	fRunningTaskCount(0),
	fLastCompletedTimerId(0)
{
	// Column list view with 3 columns
	fListView = new RepositoriesListView("list");
	fListView->SetSelectionMessage(new BMessage(LIST_SELECTION_CHANGED));
	float col0width = be_plain_font->StringWidth(kTitleEnabled) + 15;
	float col1width = be_plain_font->StringWidth(kTitleName) + 15;
	float col2width = be_plain_font->StringWidth(kTitleUrl) + 15;
	fListView->AddColumn(new BStringColumn(kTitleEnabled, col0width, col0width,
		2 * col0width, B_TRUNCATE_END), kEnabledColumn);
	fListView->AddColumn(new BStringColumn(kTitleName, 90, col1width, 300,
		B_TRUNCATE_END), kNameColumn);
	fListView->AddColumn(new BStringColumn(kTitleUrl, 500, col2width, 5000,
		B_TRUNCATE_END), kUrlColumn);
	fListView->SetInvocationMessage(new BMessage(ITEM_INVOKED));

	// Repository list status view
	fStatusContainerView = new BView("status", B_SUPPORTS_LAYOUT);
	BString templateText(kStatusViewText);
	templateText.Append(" 88");
		// Simulate a status text with two digit queue count
	fListStatusView = new BStringView("status", templateText);

	// Set a smaller fixed font size and slightly lighten text color
	BFont font(be_plain_font);
	font.SetSize(10.0f);
	fListStatusView->SetFont(&font, B_FONT_SIZE);
	fListStatusView->SetHighUIColor(fListStatusView->HighUIColor(), .9f);

	// Set appropriate explicit view sizes
	float viewWidth = std::max(fListStatusView->StringWidth(templateText),
		fListStatusView->StringWidth(kStatusCompletedText));
	BSize statusViewSize(viewWidth + 3, B_H_SCROLL_BAR_HEIGHT - 2);
	fListStatusView->SetExplicitSize(statusViewSize);
	statusViewSize.height += 1;
	fStatusContainerView->SetExplicitSize(statusViewSize);
	BLayoutBuilder::Group<>(fStatusContainerView, B_HORIZONTAL, 0)
		.Add(new BSeparatorView(B_VERTICAL))
		.AddGroup(B_VERTICAL, 0)
			.AddGlue()
			.AddGroup(B_HORIZONTAL, 0)
				.SetInsets(2, 0, 0, 0)
				.Add(fListStatusView)
				.AddGlue()
			.End()
			.Add(new BSeparatorView(B_HORIZONTAL))
		.End()
	.End();
	fListView->AddStatusView(fStatusContainerView);

	// Standard buttons
	fEnableButton = new BButton(kLabelEnable,
		new BMessage(ENABLE_BUTTON_PRESSED));
	fDisableButton = new BButton(kLabelDisable,
		new BMessage(DISABLE_BUTTON_PRESSED));

	// Create buttons with fixed size
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	int16 buttonHeight = int16(fontHeight.ascent + fontHeight.descent + 12);
		// button size determined by font size
	BSize btnSize(buttonHeight, buttonHeight);

	fAddButton = new BButton("plus", "+", new BMessage(ADD_REPO_WINDOW));
	fAddButton->SetExplicitSize(btnSize);
	fRemoveButton = new BButton("minus", "-", new BMessage(REMOVE_REPOS));
	fRemoveButton->SetExplicitSize(btnSize);

	// Layout
	int16 buttonSpacing = 1;
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_WINDOW_SPACING)
		.AddGroup(B_HORIZONTAL, 0, 0.0)
			.Add(new BStringView("instruction", B_TRANSLATE_COMMENT("Enable"
				" repositories to use with package management:",
				"Label text")), 0.0)
			.AddGlue()
		.End()
		.AddStrut(B_USE_DEFAULT_SPACING)
		.Add(fListView, 1)
		.AddGroup(B_HORIZONTAL, 0, 0.0)
			// Add and Remove buttons
			.AddGroup(B_VERTICAL, 0, 0.0)
				.AddGroup(B_HORIZONTAL, 0, 0.0)
					.Add(new BSeparatorView(B_VERTICAL))
					.AddGroup(B_VERTICAL, 0, 0.0)
						.AddGroup(B_HORIZONTAL, buttonSpacing, 0.0)
							.SetInsets(buttonSpacing)
							.Add(fAddButton)
							.Add(fRemoveButton)
						.End()
						.Add(new BSeparatorView(B_HORIZONTAL))
					.End()
					.Add(new BSeparatorView(B_VERTICAL))
				.End()
				.AddGlue()
			.End()
			// Enable and Disable buttons
			.AddGroup(B_HORIZONTAL)
				.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
					B_USE_DEFAULT_SPACING, 0)
				.AddGlue()
				.Add(fEnableButton)
				.Add(fDisableButton)
			.End()
		.End()
	.End();
}


RepositoriesView::~RepositoriesView()
{
	if (fTaskLooper) {
		fTaskLooper->Lock();
		fTaskLooper->Quit();
	}
	_EmptyList();
}


void
RepositoriesView::AllAttached()
{
	BView::AllAttached();
	fRemoveButton->SetTarget(this);
	fEnableButton->SetTarget(this);
	fDisableButton->SetTarget(this);
	fListView->SetTarget(this);
	fRemoveButton->SetEnabled(false);
	fEnableButton->SetEnabled(false);
	fDisableButton->SetEnabled(false);
	_UpdateStatusView();
	_InitList();
}


void
RepositoriesView::AttachedToWindow()
{
	fTaskLooper = new TaskLooper(BMessenger(this));
}


void
RepositoriesView::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		case REMOVE_REPOS:
		{
			RepoRow* rowItem = dynamic_cast<RepoRow*>(fListView->CurrentSelection());
			if (!rowItem || !fRemoveButton->IsEnabled())
				break;

			BString text;
			// More than one selected row
			if (fListView->CurrentSelection(rowItem)) {
				text.SetTo(B_TRANSLATE_COMMENT("Remove these repositories?",
					"Removal alert confirmation message"));
				text.Append("\n");
			}
			// Only one selected row
			else {
				text.SetTo(B_TRANSLATE_COMMENT("Remove this repository?",
					"Removal alert confirmation message"));
				text.Append("\n");
			}
			float minWidth = 0;
			while (rowItem) {
				BString repoText;
				repoText.Append("\n").Append(rowItem->Name())
					.Append(" (").Append(rowItem->Url()).Append(")");
				minWidth = std::max(minWidth, StringWidth(repoText.String()));
				text.Append(repoText);
				rowItem = dynamic_cast<RepoRow*>(fListView->CurrentSelection(rowItem));
			}
			minWidth = std::min(minWidth, Frame().Width());
				// Ensure alert window isn't much larger than the main window
			BAlert* alert = new BAlert("confirm", text, kRemoveLabel,
				kCancelLabel, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->TextView()->SetExplicitMinSize(BSize(minWidth, B_SIZE_UNSET));
			alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
			int32 answer = alert->Go();
			// User presses Cancel button
			if (answer)
				break;

			rowItem = dynamic_cast<RepoRow*>(fListView->CurrentSelection());
			while (rowItem) {
				RepoRow* oldRow = rowItem;
				rowItem = dynamic_cast<RepoRow*>(fListView->CurrentSelection(rowItem));
				fListView->RemoveRow(oldRow);
				delete oldRow;
			}
			_SaveList();
			break;
		}
		
		case LIST_SELECTION_CHANGED:
			_UpdateButtons();
			break;

		case ITEM_INVOKED:
		{
			// Simulates pressing whichever is the enabled button
			if (fEnableButton->IsEnabled()) {
				BMessage invokeMessage(ENABLE_BUTTON_PRESSED);
				MessageReceived(&invokeMessage);
			} else if (fDisableButton->IsEnabled()) {
				BMessage invokeMessage(DISABLE_BUTTON_PRESSED);
				MessageReceived(&invokeMessage);
			}
			break;
		}
		
		case ENABLE_BUTTON_PRESSED:
		{
			BStringList names;
			bool paramsOK = true;
			// Check if there are multiple selections of the same repository,
			// pkgman won't like that
			RepoRow* rowItem = dynamic_cast<RepoRow*>(fListView->CurrentSelection());
			while (rowItem) {
				if (names.HasString(rowItem->Name())
					&& kNewRepoDefaultName.Compare(rowItem->Name()) != 0) {
					(new BAlert("duplicate",
						B_TRANSLATE_COMMENT("Only one URL for each repository can "
							"be enabled.  Please change your selections.",
							"Error message"),
						kOKLabel, NULL, NULL,
						B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go(NULL);
					paramsOK = false;
					break;
				} else
					names.Add(rowItem->Name());
				rowItem = dynamic_cast<RepoRow*>(fListView->CurrentSelection(rowItem));
			}
			if (paramsOK) {
				_AddSelectedRowsToQueue();
				_UpdateButtons();
			}
			break;
		}
		
		case DISABLE_BUTTON_PRESSED:
			_AddSelectedRowsToQueue();
			_UpdateButtons();
			break;

		case TASK_STARTED:
		{
			int16 count;
			status_t result1 = message->FindInt16(key_count, &count);
			RepoRow* rowItem;
			status_t result2 = message->FindPointer(key_rowptr, (void**)&rowItem);
			if (result1 == B_OK && result2 == B_OK)
				_TaskStarted(rowItem, count);
			break;
		}
		
		case TASK_COMPLETED_WITH_ERRORS:
		{
			BString errorDetails;
			status_t result = message->FindString(key_details, &errorDetails);
			if (result == B_OK) {
				(new BAlert("error", errorDetails, kOKLabel, NULL, NULL,
					B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go(NULL);
			}
			BString repoName = message->GetString(key_name,
				kNewRepoDefaultName.String());
			int16 count;
			status_t result1 = message->FindInt16(key_count, &count);
			RepoRow* rowItem;
			status_t result2 = message->FindPointer(key_rowptr, (void**)&rowItem);
			if (result1 == B_OK && result2 == B_OK) {
				_TaskCompleted(rowItem, count, repoName);
				// Refresh the enabled status of each row since it is unsure what
				// caused the error
				_RefreshList();
			}
			_UpdateButtons();
			break;
		}
		
		case TASK_COMPLETED:
		{
			BString repoName = message->GetString(key_name,
				kNewRepoDefaultName.String());
			int16 count;
			status_t result1 = message->FindInt16(key_count, &count);
			RepoRow* rowItem;
			status_t result2 = message->FindPointer(key_rowptr, (void**)&rowItem);
			if (result1 == B_OK && result2 == B_OK) {
				_TaskCompleted(rowItem, count, repoName);
				// If the completed row has siblings then enabling this row may
				// have disabled one of the other siblings, do full refresh.
				if (rowItem->HasSiblings() && rowItem->IsEnabled())
					_RefreshList();
			}
			_UpdateButtons();
			break;
		}
		
		case TASK_CANCELED:
		{
			int16 count;
			status_t result1 = message->FindInt16(key_count, &count);
			RepoRow* rowItem;
			status_t result2 = message->FindPointer(key_rowptr, (void**)&rowItem);
			if (result1 == B_OK && result2 == B_OK)
				_TaskCanceled(rowItem, count);
			// Refresh the enabled status of each row since it is unsure what
			// caused the cancelation
			_RefreshList();
			_UpdateButtons();
			break;
		}
		
		case UPDATE_LIST:
			_RefreshList();
			_UpdateButtons();
			break;

		case STATUS_VIEW_COMPLETED_TIMEOUT:
		{
			int32 timerID;
			status_t result = message->FindInt32(key_ID, &timerID);
			if (result == B_OK && timerID == fLastCompletedTimerId)
				_UpdateStatusView();
			break;
		}
		
		default:
			BView::MessageReceived(message);
	}
}


void
RepositoriesView::_AddSelectedRowsToQueue()
{
	RepoRow* rowItem = dynamic_cast<RepoRow*>(fListView->CurrentSelection());
	while (rowItem) {
		rowItem->SetTaskState(STATE_IN_QUEUE_WAITING);
		BMessage taskMessage(DO_TASK);
		taskMessage.AddPointer(key_rowptr, rowItem);
		fTaskLooper->PostMessage(&taskMessage);
		rowItem = dynamic_cast<RepoRow*>(fListView->CurrentSelection(rowItem));
	}
}


void
RepositoriesView::_TaskStarted(RepoRow* rowItem, int16 count)
{
	fRunningTaskCount = count;
	rowItem->SetTaskState(STATE_IN_QUEUE_RUNNING);
	// Only present a status count if there is more than one task in queue
	if (count > 1) {
		_UpdateStatusView();
		fShowCompletedStatus = true;
	}
}


void
RepositoriesView::_TaskCompleted(RepoRow* rowItem, int16 count, BString& newName)
{
	fRunningTaskCount = count;
	_ShowCompletedStatusIfDone();

	// Update row state and values
	rowItem->SetTaskState(STATE_NOT_IN_QUEUE);
	if (kNewRepoDefaultName.Compare(rowItem->Name()) == 0
		&& newName.Compare("") != 0) {
		rowItem->SetName(newName.String());
		_SaveList();
	}
	_UpdateFromRepoConfig(rowItem);
}


void
RepositoriesView::_TaskCanceled(RepoRow* rowItem, int16 count)
{
	fRunningTaskCount = count;
	_ShowCompletedStatusIfDone();

	// Update row state and values
	rowItem->SetTaskState(STATE_NOT_IN_QUEUE);
	_UpdateFromRepoConfig(rowItem);
}


void
RepositoriesView::_ShowCompletedStatusIfDone()
{
	// If this is the last task show completed status text for 3 seconds
	if (fRunningTaskCount == 0 && fShowCompletedStatus) {
		fListStatusView->SetText(kStatusCompletedText);
		fLastCompletedTimerId = rand();
		BMessage timerMessage(STATUS_VIEW_COMPLETED_TIMEOUT);
		timerMessage.AddInt32(key_ID, fLastCompletedTimerId);
		new BMessageRunner(this, &timerMessage, 3000000, 1);
		fShowCompletedStatus = false;
	} else
		_UpdateStatusView();
}


void
RepositoriesView::_UpdateFromRepoConfig(RepoRow* rowItem)
{
	BPackageKit::BPackageRoster pRoster;
	BPackageKit::BRepositoryConfig repoConfig;
	BString repoName(rowItem->Name());
	status_t result = pRoster.GetRepositoryConfig(repoName, &repoConfig);
	// Repo name was found and the URL matches
	if (result == B_OK && repoConfig.BaseURL() == rowItem->Url())
		rowItem->SetEnabled(true);
	else
		rowItem->SetEnabled(false);
}


void
RepositoriesView::AddManualRepository(BString url)
{
	BUrl newRepoUrl(url);
	if (!newRepoUrl.IsValid())
		return;
	
	BString name(kNewRepoDefaultName);
	int32 index;
	int32 listCount = fListView->CountRows();
	for (index = 0; index < listCount; index++) {
		RepoRow* repoItem = dynamic_cast<RepoRow*>(fListView->RowAt(index));
		BUrl rowRepoUrl(repoItem->Url());
		// Find an already existing URL
		if (newRepoUrl == rowRepoUrl) {
			(new BAlert("duplicate",
				B_TRANSLATE_COMMENT("This repository URL already exists.",
					"Error message"),
				kOKLabel))->Go(NULL);
			return;
		}
	}
	RepoRow* newRepo = _AddRepo(name, url, false);
	_FindSiblings();
	fListView->DeselectAll();
	fListView->AddToSelection(newRepo);
	_UpdateButtons();
	_SaveList();
	if (fEnableButton->IsEnabled())
		fEnableButton->Invoke();
}


status_t
RepositoriesView::_EmptyList()
{
	BRow* row = fListView->RowAt((int32)0, NULL);
	while (row != NULL) {
		fListView->RemoveRow(row);
		delete row;
		row = fListView->RowAt((int32)0, NULL);
	}
	return B_OK;
}


void
RepositoriesView::_InitList()
{
	// Get list of known repositories from the settings file
	int32 index, repoCount;
	BStringList nameList, urlList;
	status_t result = fSettings.GetRepositories(repoCount, nameList, urlList);
	if (result == B_OK) {
		BString name, url;
		for (index = 0; index < repoCount; index++) {
			name = nameList.StringAt(index);
			url = urlList.StringAt(index);
			_AddRepo(name, url, false);
		}
	}
	_UpdateListFromRoster();
	fListView->SetSortColumn(fListView->ColumnAt(kUrlColumn), false, true);
	fListView->ResizeAllColumnsToPreferred();
}


void
RepositoriesView::_RefreshList()
{
	// Clear enabled status on all rows
	int32 index, listCount = fListView->CountRows();
	for (index = 0; index < listCount; index++) {
		RepoRow* repoItem = dynamic_cast<RepoRow*>(fListView->RowAt(index));
		if (repoItem->TaskState() == STATE_NOT_IN_QUEUE)
			repoItem->SetEnabled(false);
	}
	// Get current list of enabled repositories
	_UpdateListFromRoster();
}


void
RepositoriesView::_UpdateListFromRoster()
{
	// Get list of currently enabled repositories
	BStringList repositoryNames;
	BPackageKit::BPackageRoster pRoster;
	status_t result = pRoster.GetRepositoryNames(repositoryNames);
	if (result != B_OK) {
		(new BAlert("error",
			B_TRANSLATE_COMMENT("Repositories could not retrieve the names of "
				"the currently enabled repositories.", "Alert error message"),
			kOKLabel, NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go(NULL);
		return;
	}
	BPackageKit::BRepositoryConfig repoConfig;
	int16 index, count = repositoryNames.CountStrings();
	for (index = 0; index < count; index++) {
		const BString& repoName = repositoryNames.StringAt(index);
		result = pRoster.GetRepositoryConfig(repoName, &repoConfig);
		if (result == B_OK)
			_AddRepo(repoName, repoConfig.BaseURL(), true);
		else {
			BString text(B_TRANSLATE_COMMENT("Error getting repository"
				" configuration for %name%.", "Alert error message, "
				"do not translate %name%"));
			text.ReplaceFirst("%name%", repoName);
			(new BAlert("error", text, kOKLabel))->Go(NULL);
		}
	}
	_FindSiblings();
	_SaveList();
}


void
RepositoriesView::_SaveList()
{
	BStringList nameList, urlList;
	int32 index;
	int32 listCount = fListView->CountRows();
	for (index = 0; index < listCount; index++) {
		RepoRow* repoItem = dynamic_cast<RepoRow*>(fListView->RowAt(index));
		nameList.Add(repoItem->Name());
		urlList.Add(repoItem->Url());
	}
	fSettings.SetRepositories(nameList, urlList);
}


RepoRow*
RepositoriesView::_AddRepo(BString name, BString url, bool enabled)
{
	// URL must be valid
	BUrl repoUrl(url);
	if (!repoUrl.IsValid())
		return NULL;
	int32 index;
	int32 listCount = fListView->CountRows();
	// Find if the repo already exists in list
	for (index = 0; index < listCount; index++) {
		RepoRow* repoItem = dynamic_cast<RepoRow*>(fListView->RowAt(index));
		BUrl itemUrl(repoItem->Url());
		if (repoUrl == itemUrl) {
			// update name and enabled values
			if (name != repoItem->Name())
				repoItem->SetName(name.String());
			repoItem->SetEnabled(enabled);
			return repoItem;
		}
	}
	RepoRow* addedRow = new RepoRow(name, url, enabled);
	fListView->AddRow(addedRow);
	return addedRow;
}


void
RepositoriesView::_FindSiblings()
{
	BStringList namesFound, namesWithSiblings;
	int32 index, listCount = fListView->CountRows();
	// Find repository names that are duplicated
	for (index = 0; index < listCount; index++) {
		RepoRow* repoItem = dynamic_cast<RepoRow*>(fListView->RowAt(index));
		BString name = repoItem->Name();
		// Ignore newly added repos since we don't know the real name yet
		if (name.Compare(kNewRepoDefaultName)==0)
			continue;
		// First time a name is found- no sibling (yet)
		if (!namesFound.HasString(name))
			namesFound.Add(name);
		// Name was already found once so this name has 2 or more siblings
		else if (!namesWithSiblings.HasString(name))
			namesWithSiblings.Add(name);
	}
	// Set sibling values for each row
	for (index = 0; index < listCount; index++) {
		RepoRow* repoItem = dynamic_cast<RepoRow*>(fListView->RowAt(index));
		BString name = repoItem->Name();
		repoItem->SetHasSiblings(namesWithSiblings.HasString(name));
	}
}


void
RepositoriesView::_UpdateButtons()
{
	RepoRow* rowItem = dynamic_cast<RepoRow*>(fListView->CurrentSelection());
	// At least one row is selected
	if (rowItem) {
		bool someAreEnabled = false;
		bool someAreDisabled = false;
		bool someAreInQueue = false;
		int32 selectedCount = 0;
		RepoRow* rowItem = dynamic_cast<RepoRow*>(fListView->CurrentSelection());
		while (rowItem) {
			selectedCount++;
			uint32 taskState = rowItem->TaskState();
			if ( taskState == STATE_IN_QUEUE_WAITING
				|| taskState == STATE_IN_QUEUE_RUNNING) {
				someAreInQueue = true;
			}
			if (rowItem->IsEnabled())
				someAreEnabled = true;
			else
				someAreDisabled = true;
			rowItem = dynamic_cast<RepoRow*>(fListView->CurrentSelection(rowItem));
		}
		// Change button labels depending on which rows are selected
		if (selectedCount > 1) {
			fEnableButton->SetLabel(kLabelEnableAll);
			fDisableButton->SetLabel(kLabelDisableAll);
		} else {
			fEnableButton->SetLabel(kLabelEnable);
			fDisableButton->SetLabel(kLabelDisable);
		}
		// Set which buttons should be enabled
		fRemoveButton->SetEnabled(!someAreEnabled && !someAreInQueue);
		if ((someAreEnabled && someAreDisabled) || someAreInQueue) {
			// there are a mix of enabled and disabled repositories selected
			fEnableButton->SetEnabled(false);
			fDisableButton->SetEnabled(false);
		} else {
			fEnableButton->SetEnabled(someAreDisabled);
			fDisableButton->SetEnabled(someAreEnabled);
		}

	} else {
		// No selected rows
		fEnableButton->SetLabel(kLabelEnable);
		fDisableButton->SetLabel(kLabelDisable);
		fEnableButton->SetEnabled(false);
		fDisableButton->SetEnabled(false);
		fRemoveButton->SetEnabled(false);
	}
}


void
RepositoriesView::_UpdateStatusView()
{
	if (fRunningTaskCount) {
		BString text(kStatusViewText);
		text.Append(" ");
		text << fRunningTaskCount;
		fListStatusView->SetText(text);
	} else
		fListStatusView->SetText("");
}
