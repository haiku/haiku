// Transport Add On Template
#include "TransportAddOn.h"

class Transport : public BDataIO {
	
public:
	Transport(BDirectory* printer, BMessage* msg);
	~Transport();

	status_t InitCheck() { return B_ERROR; }

	ssize_t Read(void* buffer, size_t size);
	ssize_t Write(const void* buffer, size_t size);
};

// Impelmentation of Transport
Transport::Transport(BDirectory* printer, BMessage* msg) 
{
}

Transport::~Transport() {
}

ssize_t Transport::Read(void* buffer, size_t size) {
	return 0;
}

ssize_t Transport::Write(const void* buffer, size_t size) {
	return 0;
}

BDataIO* instanciate_transport(BDirectory* printer, BMessage* msg) {
	Transport* transport = new Transport(printer, msg);
	if (transport->InitCheck() == B_OK) {
		return transport;
	} else {
		delete transport; return NULL;
	}
}
