#ifndef _OBOSScreenSaverPreferences_H
#define _OBOSScreenSaverPreferences_H

extern const char *APP_SIG;

class OBOSScreenSaverPreferences: public BApplication {
  BWindow *m_MainForm;
public:
  OBOSScreenSaverPreferences(void);
  virtual ~OBOSScreenSaverPreferences(void);

  virtual void MessageReceived(BMessage *);

};

#endif // _OBOSScreenSaverPreferences_H
