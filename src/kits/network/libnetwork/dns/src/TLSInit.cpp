#include <TLS.h>
#include <OS.h>

// XXX: this is an ugly hack because we don't support mutexes
extern int32 gIRSInitKey;
extern int32 gGaiStrerrorKey;

class TLSInit {
	public:
		TLSInit();
};

TLSInit::TLSInit()
{
	gIRSInitKey = tls_allocate();
	gGaiStrerrorKey = tls_allocate();
}


static TLSInit __tlsinit_hack;
