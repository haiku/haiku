/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef INIT_PARAMS_PANEL_H
#define INIT_PARAMS_PANEL_H

#include "Support.h"

#include <Partition.h>
#include <PartitionParameterEditor.h>
#include <Window.h>

class BMenuField;
class BTextControl;


class InitParamsPanel : public BWindow {
public:
								InitParamsPanel(BWindow* window,
									const BString& diskSystem,
									BPartition* partition);
	virtual						~InitParamsPanel();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

			int32				Go(BString& name, BString& parameters);
			void				Cancel();

private:
	class EscapeFilter;
			EscapeFilter*		fEscapeFilter;
			sem_id				fExitSemaphore;
			BWindow*			fWindow;
			BButton*			fOkButton;
			int32				fReturnValue;

			BPartitionParameterEditor*	fEditor;
};

#endif // INIT_PARAMS_PANEL_H
