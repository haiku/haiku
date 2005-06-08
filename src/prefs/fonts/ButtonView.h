/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef BUTTON_VIEW_H
#define BUTTON_VIEW_H
	
#include <View.h>
#include <Button.h>

// Message sent when the rescan button is sent.
#define RESCAN_FONTS_MSG 'rscn'

// Message sent when the reset button is sent.
#define RESET_FONTS_MSG 'rset'

// Message sent when the revert button is sent.
#define REVERT_MSG 'rvrt'
	
class ButtonView : public BView 
{
public:
			ButtonView(BRect frame);
	bool	RevertState();
	void	SetRevertState(bool b);
	void	Draw(BRect);
	
private:

	BButton	*revertButton;
};
	
#endif
