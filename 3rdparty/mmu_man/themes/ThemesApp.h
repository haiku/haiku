#include <Application.h>

class ThemesApp : public BApplication {
public:
	ThemesApp();
	virtual ~ThemesApp();
	void ReadyToRun();
	void MessageReceived(BMessage *message);

private:
};
