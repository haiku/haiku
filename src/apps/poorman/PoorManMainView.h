/* PoorManMainView.h
 *
 *	Philip Harrison
 *	Started: 5/13/2004
 *	Version: 0.1
 */
 
#ifndef POOR_MAN_MAIN_VIEW_H
#define POOR_MAN_MAIN_VIEW_H

#include <View.h>

class PoorManMainView: public BView
{
public:
				PoorManMainView(BRect, char *name);
virtual	void	AttachedToWindow();
virtual	void	Draw(BRect updateRect);
};

#endif
