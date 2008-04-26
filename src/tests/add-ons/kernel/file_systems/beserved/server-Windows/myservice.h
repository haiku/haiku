// myservice.h

#include "ntservice.h"


#define SERVICE_USER_CONTROL_HUP		SERVICE_CONTROL_USER + 1


class CMyService : public CNTService
{
public:
	CMyService();
	virtual BOOL OnInit();
	virtual void OnStop();
	virtual BOOL Run();
	virtual void Restart();
	virtual BOOL OnUserControl(DWORD dwOpcode);
	BOOL IsRunning();

private:
	// Initialization
	void InitLogging();
};
