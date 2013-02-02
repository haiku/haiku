/*
 * Copyright 2008-2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Bryce Groff	  <bgroff@hawaii.edu>
 */
#ifndef CREATE_PARAMS_PANEL_H
#define CREATE_PARAMS_PANEL_H


#include "AbstractParametersPanel.h"


class BMenuField;
class BPopUpMenu;
class BTextControl;
class SizeSlider;


class CreateParametersPanel : public AbstractParametersPanel {
public:
								CreateParametersPanel(BWindow* window,
									BPartition* parent, off_t offset,
									off_t size);
	virtual						~CreateParametersPanel();

			status_t			Go(off_t& offset, off_t& size, BString& name,
									BString& type, BString& parameters);

	virtual	void				MessageReceived(BMessage* message);

protected:
	virtual	void				AddControls(BLayoutBuilder::Group<>& builder,
									BView* editorView);

private:
			void 				_CreateViewControls(BPartition* parent,
									off_t offset, off_t size);

			void				_UpdateSizeTextControl();

private:
			BPopUpMenu*			fTypePopUpMenu;
			BMenuField*			fTypeMenuField;
			BTextControl*		fNameTextControl;
			SizeSlider*			fSizeSlider;
			BTextControl*		fSizeTextControl;
};


#endif // CREATE_PARAMS_PANEL_H
