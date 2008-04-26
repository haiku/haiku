// ScanPan.h

#include "ctrlpan.h"

#define MYSERVICENAME "Bar Code Scanner Control"

class CScanPanel : public CControlPanel
{
public:
	virtual LONG OnInquire(UINT uAppNum,NEWCPLINFO *pInfo); 
	virtual LONG OnDblclk(HWND hwndCPl,UINT uAppNum,LONG lData);
};
