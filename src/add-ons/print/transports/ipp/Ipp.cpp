// Sun, 18 Jun 2000
// Y.Takagi

#include "IppTransport.h"
#include "DbgMsg.h"

#include <PrintTransportAddOn.h>
#include <NetEndpoint.h>
#include <String.h>
#include <OS.h>

#include <HashString.h>
#include <HashMap.h>

IppTransport *transport = NULL;

// Set transport_features so we stay loaded
uint32 transport_features = B_TRANSPORT_IS_HOTPLUG | B_TRANSPORT_IS_NETWORK;


class IPPPrinter {
public:
	IPPPrinter(const BString& uri, uint32 type)
		{ fURI=uri; fType=type; }

	uint32 fType, fState;
	BString fURI, fLocation, fInfo, fMakeModel, fAttributes;
};


class IPPPrinterRoster {
public:
	IPPPrinterRoster();
	~IPPPrinterRoster();

	status_t ListPorts(BMessage *msg);
	status_t Listen();
private:
	char *_ParseString(BString& outStr, char*& pos);

	static status_t _IPPListeningThread(void *cookie);

	typedef HashMap<HashString,IPPPrinter*> IPPPrinterMap;
	IPPPrinterMap fPrinters;
	BNetEndpoint *fEndpoint;
	thread_id fListenThreadID;
};


static IPPPrinterRoster gRoster;


IPPPrinterRoster::IPPPrinterRoster()
{
	// Setup our (UDP) listening endpoint
	fEndpoint = new BNetEndpoint(SOCK_DGRAM);
	if (!fEndpoint)
		return;

	if (fEndpoint->InitCheck() != B_OK)
		return;

	if (fEndpoint->Bind(BNetAddress(INADDR_ANY, 631)) != B_OK)
		return;

	// Now create thread for listening
	fListenThreadID = spawn_thread(_IPPListeningThread, "IPPListener", B_LOW_PRIORITY, this);
	if (fListenThreadID <= 0)
		return;

	resume_thread(fListenThreadID);
}


IPPPrinterRoster::~IPPPrinterRoster()
{
	kill_thread(fListenThreadID);
	delete fEndpoint;
}


status_t
IPPPrinterRoster::Listen()
{
	BNetAddress srcAddress;
	uint32 type, state;
	char packet[1541];
	char uri[256];
	char* pos;
	int32 len;

	while ((len=fEndpoint->ReceiveFrom(packet, sizeof(packet) -1, srcAddress)) > 0) {
		packet[len] = '\0';

		// Verify packet format
		if (sscanf(packet, "%lx%lx%1023s", &type, &state, uri) == 3) {
			IPPPrinter *printer = fPrinters.Get(uri);
			if (!printer) {
				printer = new IPPPrinter(uri, type);
				fPrinters.Put(printer->fURI.String(), printer);
			}

			printer->fState=state;

			// Check for option parameters
			if ((pos=strchr(packet, '"')) != NULL) {
				BString str;
				if (_ParseString(str, pos))
					printer->fLocation = str;
				if (pos && _ParseString(str, pos))
					printer->fInfo = str;
				if (pos && _ParseString(str, pos))
					printer->fMakeModel = str;

				if (pos)
					printer->fAttributes = pos;
			}

			DBGMSG(("Printer: %s\nLocation: %s\nInfo: %s\nMakeModel: %s\nAttributes: %s\n",
				printer->fURI.String(), printer->fLocation.String(), printer->fInfo.String(),
				printer->fMakeModel.String(), printer->fAttributes.String()));
		}
	}

	return len;
}


status_t
IPPPrinterRoster::ListPorts(BMessage* msg)
{
	IPPPrinterMap::Iterator iterator = fPrinters.GetIterator();
	while (iterator.HasNext()) {
		const IPPPrinterMap::Entry& entry = iterator.Next();
		msg->AddString("port_id", entry.value->fURI);

		BString name = entry.value->fInfo;
		if (name.Length() && entry.value->fLocation.Length()) {
			name.Append(" [");
			name.Append(entry.value->fLocation);
			name.Append("]");
		}

		msg->AddString("port_name", name);
	}

	return B_OK;
}


char*
IPPPrinterRoster::_ParseString(BString& outStr, char*& pos)
{
	outStr = "";

	if (*pos == '"')
		pos++;

	while(*pos && *pos != '"')
		outStr.Append(*pos++, 1);

	if (*pos == '"')
		++pos;

	while(*pos && isspace(*pos))
		++pos;

	if (!*pos)
		pos = NULL;

	return pos;
}


status_t
IPPPrinterRoster::_IPPListeningThread(void *cookie)
{
	return ((IPPPrinterRoster*)cookie)->Listen();
}


// --- general transport hooks

extern "C" _EXPORT void
exit_transport()
{
	DBGMSG(("> exit_transport\n"));
	if (transport) {
		delete transport;
		transport = NULL;
	}
	DBGMSG(("< exit_transport\n"));
}


// List detected printers
extern "C" _EXPORT status_t
list_transport_ports(BMessage* msg)
{
	return gRoster.ListPorts(msg);
}


extern "C" _EXPORT BDataIO *
init_transport(BMessage *msg)
{
	DBGMSG(("> init_transport\n"));

	transport = new IppTransport(msg);

	if (transport->fail()) {
		exit_transport();
	}

	if (msg)
		msg->what = 'okok';

	DBGMSG(("< init_transport\n"));
	return transport;
}
