/* PoorManView.h
 *
 *	Philip Harrison
 *	Started: 4/25/2004
 *	Version: 0.1
 */
 
#ifndef POOR_MAN_VIEW_H
#define POOR_MAN_VIEW_H

#ifndef _VIEW_H 
#include <View.h>
#endif

class PoorManView: public BView {
public:
							PoorManView(BRect, const char *name);

	virtual	void		AttachedToWindow();
};

#endif
