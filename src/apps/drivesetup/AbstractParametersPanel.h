/*
 * Copyright 2008-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de.
 */
#ifndef ABSTRACT_PARAMETERS_PANEL_H
#define ABSTRACT_PARAMETERS_PANEL_H


#include <LayoutBuilder.h>
#include <Partition.h>
#include <PartitionParameterEditor.h>
#include <Window.h>

#include "Support.h"


class BMenuField;
class BTextControl;


class AbstractParametersPanel : public BWindow {
public:
								AbstractParametersPanel(BWindow* window);
	virtual						~AbstractParametersPanel();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

			status_t			Go(BString& parameters);
			void				Cancel();

protected:
			void				Init(B_PARAMETER_EDITOR_TYPE type,
									const BString& diskSystem,
									BPartition* partition);
			status_t			Go(BString& parameters, BMessage& storage);

	virtual bool				NeedsEditor() const;
	virtual bool				ValidWithoutEditor() const;
	virtual status_t			ParametersReceived(const BString& parameters,
									BMessage& storage);
	virtual	void				AddControls(BLayoutBuilder::Group<>& builder,
									BView* editorView);

protected:
			BButton*			fOkButton;
			status_t			fReturnStatus;

			BPartitionParameterEditor* fEditor;

private:
	class EscapeFilter;
			EscapeFilter*		fEscapeFilter;
			sem_id				fExitSemaphore;
			BWindow*			fWindow;
};


#endif // ABSTRACT_PARAMETERS_PANEL_H
