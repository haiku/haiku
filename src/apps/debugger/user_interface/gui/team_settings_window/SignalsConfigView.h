/*
 * Copyright 2013-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SIGNALS_CONFIG_VIEW_H
#define SIGNALS_CONFIG_VIEW_H


#include <GroupView.h>

#include "table/Table.h"

#include "Team.h"

#include "types/Types.h"


class BButton;
class BMenuField;
class SignalDispositionEditWindow;
class UserInterfaceListener;


class SignalsConfigView : public BGroupView, private Team::Listener,
	private TableListener {
public:
								SignalsConfigView(::Team* team,
									UserInterfaceListener* listener);

								~SignalsConfigView();

	static	SignalsConfigView* 	Create(::Team* team,
									UserInterfaceListener* listener);
									// throws

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			class SignalDispositionModel;

private:
	// Team::Listener
	virtual	void				CustomSignalDispositionChanged(
									const Team::CustomSignalDispositionEvent&
										event);
	virtual	void				CustomSignalDispositionRemoved(
									const Team::CustomSignalDispositionEvent&
										event);

	// TableListener
	virtual	void				TableSelectionChanged(Table* table);

			void	 			_Init();

			void				_UpdateSignalConfigState();
									// must be called with team lock held

private:
			::Team*				fTeam;
			UserInterfaceListener* fListener;
			BMenuField*			fDefaultSignalDisposition;
			Table*				fDispositionExceptions;
			BButton*			fAddDispositionButton;
			BButton*			fEditDispositionButton;
			BButton*			fRemoveDispositionButton;
			SignalDispositionModel* fDispositionModel;
			SignalDispositionEditWindow* fEditWindow;
};


#endif // SIGNALS_CONFIG_VIEW_H
