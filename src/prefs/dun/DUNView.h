/*

DUNView Header - DialUp Networking View Header

Authors: Sikosis (beos@gravity24hr.com)
		 Misza (misza@ihug.com.au) 

(C) 2002 OpenBeOS under MIT license

*/

#ifndef __DUNVIEW_H__
#define __DUNVIEW_H__

class DUNView : public BView
{
	public:
   		DUNView(BRect frame);
};

class ModemView : public BView
{
	public:
		ModemView(BRect frame);
};

class SettingsView : public BView
{
	public:
		SettingsView(BRect frame);
};

class NewConnectionWindowView : public BView
{
	public:
		NewConnectionWindowView(BRect frame);
};

#endif
