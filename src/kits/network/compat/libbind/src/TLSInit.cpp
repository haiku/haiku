#include <pthread_emu.h>

// XXX: this is an ugly hack because we don't support mutexes
extern pthread_key_t gIRSInitKey;
extern pthread_key_t gGaiStrerrorKey;

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
