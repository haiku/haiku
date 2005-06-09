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
#define M_RESCAN_FONTS 'rscn'
	
class ButtonView : public BView 
{
public:
			ButtonView(BRect frame);
	bool	RevertState() const;
	void	SetRevertState(bool b);
	void	Draw(BRect);
	
private:

	BButton	*fRevertButton;
};
	
#endif
