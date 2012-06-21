/*
 * Copyright 2008-2012 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Bryce Groff	  <bgroff@hawaii.edu>
 */
#ifndef CREATE_PARAMS_PANEL_H
#define CREATE_PARAMS_PANEL_H


#include <Window.h>
#include <InterfaceKit.h>
#include <PartitionParameterEditor.h>
#include <Partition.h>


class BMenuField;
class BTextControl;
class SizeSlider;


class CreateParamsPanel : public BWindow {
public:
								CreateParamsPanel(BWindow* window,
									BPartition* parent, off_t offset,
									off_t size);
	virtual						~CreateParamsPanel();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

			int32				Go(off_t& offset, off_t& size, BString& name,
									BString& type, BString& parameters);
			void				Cancel();


private:
			void 				_CreateViewControls(BPartition* parent,
									off_t offset, off_t size);

			void				_UpdateSizeTextControl();

private:
	class EscapeFilter;

			EscapeFilter*		fEscapeFilter;
			sem_id				fExitSemaphore;
			BWindow*			fWindow;
			int32				fReturnValue;

			BPartitionParameterEditor*	fEditor;

			BPopUpMenu*			fTypePopUpMenu;
			BMenuField*			fTypeMenuField;
			BTextControl*		fNameTextControl;
			SizeSlider*			fSizeSlider;
			BTextControl*		fSizeTextControl;
};


#endif // CREATE_PARAMS_PANEL_H
