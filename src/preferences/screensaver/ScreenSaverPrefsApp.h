#ifndef _ScreenSaverPrefsApp_H
#define _ScreenSaverPrefsApp_H
#include "ScreenSaverWindow.h"

extern const char *APP_SIG;

class ScreenSaverPrefsApp: public BApplication {
private:
  ScreenSaverWin *m_MainForm;
public:
  ScreenSaverPrefsApp(void);
  virtual ~ScreenSaverPrefsApp(void);
  virtual void MessageReceived(BMessage *);
  virtual void RefsReceived(BMessage *);

};

#endif // _ScreenSaverPrefsApp_H
