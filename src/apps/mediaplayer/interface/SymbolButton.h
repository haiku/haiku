/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */
#ifndef SYMBOL_BUTTON_H
#define SYMBOL_BUTTON_H


#include <Button.h>
#include <ControlLook.h>

class BShape;


class SymbolButton : public BButton {
public:
								SymbolButton(const char* name,
									BShape* symbolShape,
									BMessage* message = NULL,
									uint32 borders
										= BControlLook::B_ALL_BORDERS);

	virtual						~SymbolButton();

	// BButton interface
	virtual	void				Draw(BRect updateRect);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

	// SymbolButton
			void				SetSymbol(BShape* symbolShape);

private:
			BShape*				fSymbol;
			uint32				fBorders;
};


#endif	// SYMBOL_BUTTON_H
