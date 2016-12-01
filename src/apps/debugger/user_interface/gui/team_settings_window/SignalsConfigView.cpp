/*
 * Copyright 2013-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "SignalsConfigView.h"

#include <Box.h>
#include <Button.h>
#include <LayoutBuilder.h>
#include <MenuField.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "table/TableColumns.h"

#include "AppMessageCodes.h"
#include "MessageCodes.h"
#include "SignalDispositionEditWindow.h"
#include "SignalDispositionMenu.h"
#include "SignalDispositionTypes.h"
#include "UiUtils.h"
#include "UserInterface.h"


enum {
	MSG_ADD_DISPOSITION_EXCEPTION			= 'adex',
	MSG_EDIT_DISPOSITION_EXCEPTION			= 'edex',
	MSG_REMOVE_DISPOSITION_EXCEPTION		= 'rdex'
};


struct SignalDispositionInfo {
	SignalDispositionInfo(int32 _signal, int32 _disposition)
		:
		signal(_signal),
		disposition(_disposition)
	{
	}

	int32 signal;
	int32 disposition;
};



class SignalsConfigView::SignalDispositionModel : public TableModel {
public:

	SignalDispositionModel(::Team* team)
		:
		fDispositions(),
		fTeam(team)
	{
	}

	virtual ~SignalDispositionModel()
	{
	}

	virtual int32 CountColumns() const
	{
		return 2;
	}

	virtual int32 CountRows() const
	{
		return fDispositions.CountItems();
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& value)
	{
		SignalDispositionInfo* info = fDispositions.ItemAt(rowIndex);
		if (info == NULL)
			return false;

		switch (columnIndex) {
			case 0:
			{
				BString tempValue;
				value.SetTo(UiUtils::SignalNameToString(info->signal,
					tempValue));
				return true;
			}
			case 1:
			{
				value.SetTo(UiUtils::SignalDispositionToString(
						info->disposition), B_VARIANT_DONT_COPY_DATA);
				return true;
			}
			default:
				break;
		}

		return false;
	}

	bool SignalDispositionInfoAt(int32 rowIndex, SignalDispositionInfo*& _info)
	{
		_info = fDispositions.ItemAt(rowIndex);
		if (_info == NULL)
			return false;

		return true;
	}

	void Update(int32 signal, int32 disposition)
	{
		for (int32 i = 0; i < fDispositions.CountItems(); i++) {
			SignalDispositionInfo* info = fDispositions.ItemAt(i);
			if (info->signal == signal) {
				info->disposition = disposition;
				NotifyRowsChanged(i, 1);
				return;
			}
		}

		SignalDispositionInfo* info = new(std::nothrow) SignalDispositionInfo(
			signal, disposition);
		if (info == NULL)
			return;

		ObjectDeleter<SignalDispositionInfo> infoDeleter(info);
		if (!fDispositions.AddItem(info))
			return;

		infoDeleter.Detach();
		NotifyRowsAdded(fDispositions.CountItems() - 1, 1);
	}

	void Remove(int32 signal)
	{
		for (int32 i = 0; i < fDispositions.CountItems(); i++) {
			SignalDispositionInfo* info = fDispositions.ItemAt(i);
			if (info->signal == signal) {
				fDispositions.RemoveItemAt(i);
				delete info;
				NotifyRowsRemoved(i, 1);
				return;
			}
		}
	}

	status_t Init()
	{
		AutoLocker< ::Team> locker(fTeam);

		const SignalDispositionMappings& dispositions
			= fTeam->GetSignalDispositionMappings();

		for (SignalDispositionMappings::const_iterator it
			= dispositions.begin(); it != dispositions.end(); ++it) {
			SignalDispositionInfo* info
				= new(std::nothrow) SignalDispositionInfo(it->first,
				it->second);
			if (info == NULL)
				return B_NO_MEMORY;

			ObjectDeleter<SignalDispositionInfo> infoDeleter(info);

			if (!fDispositions.AddItem(info))
				return B_NO_MEMORY;

			infoDeleter.Detach();
		}

		return B_OK;
	}

private:
	typedef BObjectList<SignalDispositionInfo> DispositionList;

private:
	DispositionList		fDispositions;
	::Team*				fTeam;
};


SignalsConfigView::SignalsConfigView(::Team* team,
	UserInterfaceListener* listener)
	:
	BGroupView(B_VERTICAL),
	fTeam(team),
	fListener(listener),
	fDefaultSignalDisposition(NULL),
	fDispositionExceptions(NULL),
	fAddDispositionButton(NULL),
	fEditDispositionButton(NULL),
	fRemoveDispositionButton(NULL),
	fDispositionModel(NULL),
	fEditWindow(NULL)
{
	SetName("Signals");
	fTeam->AddListener(this);
}


SignalsConfigView::~SignalsConfigView()
{
	fTeam->RemoveListener(this);
	BMessenger(fEditWindow).SendMessage(B_QUIT_REQUESTED);
}


SignalsConfigView*
SignalsConfigView::Create(::Team* team, UserInterfaceListener* listener)
{
	SignalsConfigView* self = new SignalsConfigView(team, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
SignalsConfigView::AttachedToWindow()
{
	fDefaultSignalDisposition->Menu()->SetTargetForItems(this);
	fAddDispositionButton->SetTarget(this);
	fEditDispositionButton->SetTarget(this);
	fRemoveDispositionButton->SetTarget(this);

	fEditDispositionButton->SetEnabled(false);
	fRemoveDispositionButton->SetEnabled(false);

	AutoLocker< ::Team> teamLocker(fTeam);
	_UpdateSignalConfigState();

	BGroupView::AttachedToWindow();
}


void
SignalsConfigView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SIGNAL_DISPOSITION_EDIT_WINDOW_CLOSED:
		{
			fEditWindow = NULL;
		}
		case MSG_SET_DEFAULT_SIGNAL_DISPOSITION:
		{
			int32 disposition;
			if (message->FindInt32("disposition", &disposition) != B_OK)
				break;

			fListener->SetDefaultSignalDispositionRequested(disposition);
			break;
		}
		case MSG_ADD_DISPOSITION_EXCEPTION:
		case MSG_EDIT_DISPOSITION_EXCEPTION:
		{
			if (fEditWindow != NULL) {
				AutoLocker<BWindow> lock(fEditWindow);
				if (lock.IsLocked())
					fEditWindow->Activate(true);
			} else {
				int32 signal = 0;
				if (message->what == MSG_EDIT_DISPOSITION_EXCEPTION) {
					TableSelectionModel* model
						= fDispositionExceptions->SelectionModel();
					SignalDispositionInfo* info;
					if (fDispositionModel->SignalDispositionInfoAt(
							model->RowAt(0), info)) {
						signal = info->signal;
					}
				}

				try {
					fEditWindow = SignalDispositionEditWindow::Create(fTeam,
						signal, fListener, this);
					if (fEditWindow != NULL)
						fEditWindow->Show();
	           	} catch (...) {
	           		// TODO: notify user
	           	}
			}
			break;
		}
		case MSG_REMOVE_DISPOSITION_EXCEPTION:
		{
			TableSelectionModel* model
				= fDispositionExceptions->SelectionModel();
			for (int32 i = 0; i < model->CountRows(); i++) {
				SignalDispositionInfo* info;
				if (fDispositionModel->SignalDispositionInfoAt(model->RowAt(i),
						info)) {
					fListener->RemoveCustomSignalDispositionRequested(
						info->signal);
				}
			}
			break;
		}
		case MSG_SET_CUSTOM_SIGNAL_DISPOSITION:
		{
			int32 signal;
			int32 disposition;
			if (message->FindInt32("signal", &signal) == B_OK
				&& message->FindInt32("disposition", &disposition) == B_OK) {
				fDispositionModel->Update(signal, disposition);
			}
			break;
		}
		case MSG_REMOVE_CUSTOM_SIGNAL_DISPOSITION:
		{
			int32 signal;
			if (message->FindInt32("signal", &signal) == B_OK)
				fDispositionModel->Remove(signal);
			break;
		}
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
SignalsConfigView::CustomSignalDispositionChanged(
	const Team::CustomSignalDispositionEvent& event)
{
	BMessage message(MSG_SET_CUSTOM_SIGNAL_DISPOSITION);
	message.AddInt32("signal", event.Signal());
	message.AddInt32("disposition", event.Disposition());

	BMessenger(this).SendMessage(&message);
}


void
SignalsConfigView::CustomSignalDispositionRemoved(
	const Team::CustomSignalDispositionEvent& event)
{
	BMessage message(MSG_REMOVE_CUSTOM_SIGNAL_DISPOSITION);
	message.AddInt32("signal", event.Signal());

	BMessenger(this).SendMessage(&message);
}


void
SignalsConfigView::TableSelectionChanged(Table* table)
{
	TableSelectionModel* model = fDispositionExceptions->SelectionModel();
	int32 rowCount = model->CountRows();
	fEditDispositionButton->SetEnabled(rowCount == 1);
	fRemoveDispositionButton->SetEnabled(rowCount > 0);
}


void
SignalsConfigView::_Init()
{
	SignalDispositionMenu* dispositionMenu = new SignalDispositionMenu(
		"signalDispositionsMenu",
		new BMessage(MSG_SET_DEFAULT_SIGNAL_DISPOSITION));

	BGroupView* customDispositionsGroup = new BGroupView();
	BLayoutBuilder::Group<>(customDispositionsGroup, B_VERTICAL, 0.0)
		.SetInsets(B_USE_SMALL_INSETS)
		.Add(fDispositionExceptions = new Table("customDispositions",
			B_WILL_DRAW, B_FANCY_BORDER))
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_SMALL_INSETS)
			.AddGlue()
			.Add(fAddDispositionButton = new BButton("Add" B_UTF8_ELLIPSIS,
				new BMessage(MSG_ADD_DISPOSITION_EXCEPTION)))
			.Add(fEditDispositionButton = new BButton("Edit" B_UTF8_ELLIPSIS,
				new BMessage(MSG_EDIT_DISPOSITION_EXCEPTION)))
			.Add(fRemoveDispositionButton = new BButton("Remove",
				new BMessage(MSG_REMOVE_DISPOSITION_EXCEPTION)))
		.End();

	BBox* dispositionsBox = new BBox("custom dispositions box");
	dispositionsBox->AddChild(customDispositionsGroup);
	dispositionsBox->SetLabel("Custom dispositions");

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fDefaultSignalDisposition = new BMenuField(
			"stopTypes", "Default disposition:", dispositionMenu))
		.Add(dispositionsBox);

	dispositionMenu->SetLabelFromMarked(true);

	// columns
	fDispositionExceptions->AddColumn(new StringTableColumn(0, "Signal",
		80, 40, 1000, B_TRUNCATE_END, B_ALIGN_LEFT));
	fDispositionExceptions->AddColumn(new StringTableColumn(1, "Disposition",
		200, 40, 1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fDispositionExceptions->SetExplicitMinSize(BSize(400.0, 200.0));


	fDispositionModel = new SignalDispositionModel(fTeam);
	if (fDispositionModel->Init() != B_OK) {
		delete fDispositionModel;
		return;
	}

	fDispositionExceptions->SetTableModel(fDispositionModel);
	fDispositionExceptions->AddTableListener(this);
}


void
SignalsConfigView::_UpdateSignalConfigState()
{
	int32 defaultDisposition = fTeam->DefaultSignalDisposition();
	fDefaultSignalDisposition->Menu()->ItemAt(defaultDisposition)
		->SetMarked(true);
}
