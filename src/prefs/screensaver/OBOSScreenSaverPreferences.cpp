#include <Application.h>

#include "OBOSScreenSaverPreferences.h"
#include "ScreenSaver.h"

const char *APP_SIG = "application/x-vnd.SSPreferences";

OBOSScreenSaverPreferences::OBOSScreenSaverPreferences(void) : BApplication(APP_SIG)
{
  m_MainForm = new ScreenSaver();
  m_MainForm->Show();
}

OBOSScreenSaverPreferences::~OBOSScreenSaverPreferences(void)
{
}

void OBOSScreenSaverPreferences::MessageReceived(BMessage *message)
{
  switch(message->what)
  {
    case B_READY_TO_RUN: 
			break;
    default:
    {
      BApplication::MessageReceived(message);
    }
  }
}

int main(void)
{
  OBOSScreenSaverPreferences app;
  app.Run();
  return 0;
}
