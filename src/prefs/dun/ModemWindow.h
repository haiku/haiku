/*

ModemWindow Header by Sikosis (beos@gravity24hr.com)

(C) 2002 OpenBeOS under MIT license

*/

#ifndef __MODEMWINDOW_H__
#define __MODEMWINDOW_H__

class ModemView; 

class ModemWindow : public BWindow
{
public:
   ModemWindow(BRect frame);
   ~ModemWindow();
   virtual void MessageReceived(BMessage *message);
private:
   void InitWindow(void);
   ModemView* aModemview;
   
   BButton *btnModemWindowCustom;
   BButton *btnModemWindowCancel;
   BButton *btnModemWindowDone;
   
   BMenu *YourModemIsMenu;
   BMenuField *YourModemIsMenuField;
   BMenu *ConnectViaMenu;
   BMenuField *ConnectViaMenuField;
   BMenu *SpeedMenu;
   BMenuField *SpeedMenuField;
   
   BTextView *tvRedial;
   BTextView *tvTimesBusySignal;
   BTextView *tvReadLogPath;
   BTextView *tvWriteLogPath;
   
   BCheckBox *chkMakeConnection;
   BCheckBox *chkShowTerminal;
   BCheckBox *chkLogAll;
};

#endif
