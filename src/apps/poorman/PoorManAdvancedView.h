/* PoorManAdvancedView.h
 *
 *	Philip Harrison
 *	Started: 5/12/2004
 *	Version: 0.1
 */
 
#ifndef POOR_MAN_ADVANCED_VIEW_H
#define POOR_MAN_ADVANCED_VIEW_H

#include <View.h>

#include "StatusSlider.h"


class PoorManAdvancedView: public BView
{
public:
				PoorManAdvancedView(BRect, const char *name);
		int32	MaxSimultaneousConnections() { return maxConnections->Value(); }
		void	SetMaxSimutaneousConnections(int32 num);
private:
		// Advanced Tab
			// Connections Options
			StatusSlider	*	maxConnections;
};

#endif
