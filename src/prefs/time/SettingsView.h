/*
	
	SettingsView.h
*/

#ifndef SETTINGS_VIEW_H
#define SETTINGS_VIEW_H

#ifndef _VIEW_H
#include <View.h>
#endif

class SettingsView : public BView
{
public:
						SettingsView(BRect frame);
		virtual void	Draw(BRect frame);
				void	changeRTCsetting();
		
				
private:
		void			buildView();
		
};

#endif