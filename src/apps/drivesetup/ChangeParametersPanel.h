/*
 * Copyright 2008-2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Bryce Groff	  <bgroff@hawaii.edu>
 */
#ifndef CHANGE_PARAMS_PANEL_H
#define CHANGE_PARAMS_PANEL_H


#include "AbstractParametersPanel.h"


class BMenuField;
class BPopUpMenu;
class BTextControl;


class ChangeParametersPanel : public AbstractParametersPanel {
public:
								ChangeParametersPanel(BWindow* window,
									BPartition* parent);
	virtual						~ChangeParametersPanel();

			status_t			Go(BString& name, BString& type,
									BString& parameters);

	virtual	void				MessageReceived(BMessage* message);

protected:
								ChangeParametersPanel(BWindow* window);

			status_t			Go(BString& name, BString& type,
									BString& parameters, BMessage& storage);
			void 				CreateChangeControls(BPartition* parent);

	virtual bool				NeedsEditor() const;
	virtual bool				ValidWithoutEditor() const;
	virtual status_t			ParametersReceived(const BString& parameters,
									BMessage& storage);
	virtual	void				AddControls(BLayoutBuilder::Group<>& builder,
									BView* editorView);

protected:
			BPopUpMenu*			fTypePopUpMenu;
			BMenuField*			fTypeMenuField;
			BTextControl*		fNameTextControl;
			bool				fSupportsName;
			bool				fSupportsType;
};


#endif // CHANGE_PARAMS_PANEL_H
