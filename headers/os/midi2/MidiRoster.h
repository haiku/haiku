
#ifndef _MIDI_ROSTER_H
#define _MIDI_ROSTER_H

#include <Application.h>
#include <MidiEndpoint.h>

enum BMidiOp
{
	B_MIDI_NO_OP,
	B_MIDI_REGISTERED,
	B_MIDI_UNREGISTERED,
	B_MIDI_CONNECTED,
	B_MIDI_DISCONNECTED,
	B_MIDI_CHANGED_NAME,
	B_MIDI_CHANGED_LATENCY,
	B_MIDI_CHANGED_PROPERTIES
};

#define B_MIDI_EVENT 'MIDI'

class BMidiProducer;
class BMidiConsumer;
class BMidiRosterLooper;

class BMidiRoster
{
public:
	static BMidiEndpoint *NextEndpoint(int32 *id);
	static BMidiProducer *NextProducer(int32 *id);
	static BMidiConsumer *NextConsumer(int32 *id);
	
	static BMidiEndpoint *FindEndpoint(int32 id, bool local_only = false);
	static BMidiProducer *FindProducer(int32 id, bool local_only = false);
	static BMidiConsumer *FindConsumer(int32 id, bool local_only = false);
	// if local_only is true only local objects will be resolved
	
	static void StartWatching(const BMessenger *msngr);
	static void StopWatching();
	//   what =     B_MIDI_EVENT (for all below)
	//
	//   be:op =       B_MIDI_REGISTERED or B_MIDI_UNREGISTERED
	//   be:id =       <id>
	//   be:type =     "producer" | "consumer"
	//
	//   be:op =       B_MIDI_CHANGED_NAME
	//   be:id =       <id>
	//   be:type =     "producer" | "consumer"
	//   be:name =     <name>
	//   
	//   be:op =       B_MIDI_CHANGED_LATENCY
	//   be:id =       <id>
	//   be:type =     "producer" | "consumer"
	//   be:latency =  <latency>
	//
	//   be:op =       B_MIDI_CHANGED_PROPERTIES
	//   be:id =       <id>
	//   be:type =     "producer" | "consumer"
	//   be:properties = <properties>
	//
	//   be:op =       B_MIDI_CONNECTED     (when source->Connect(sink) happens)
	//   be:producer = <id>  - connector id (0 if not a public object)
	//   be:consumer = <id>  - connectee id (0 if not a public object)
	//
	//   be:op =       B_MIDI_DISCONNECTED  (when source->Connect(sink) happens)
	//   be:producer = <id>  - connector id (0 if not a public object)
	//   be:consumer = <id>  - connectee id (0 if not a public object)
	//   
	// When you StartWatching(), you will receive added 
	// events for all existing midi objects
				
	static status_t Register(BMidiEndpoint *object);
	static status_t Unregister(BMidiEndpoint *object);
	// Publish a midi object so that it is externall visible.

	static BMidiRoster *MidiRoster();
	
private:
	friend class BMidiEndpoint;
	friend class BMidiProducer;
	friend class BMidiConsumer;
	friend class BMidiLocalProducer;
	friend class BMidiLocalConsumer;
	friend class BMidiRosterLooper;
	friend class MidiRosterApp;
		
	BMidiRoster(BMessenger *remote = NULL);
	virtual ~BMidiRoster();

	virtual void _Reserved1();
	virtual void _Reserved2();
	virtual void _Reserved3();
	virtual void _Reserved4();
	virtual void _Reserved5();
	virtual void _Reserved6();
	virtual void _Reserved7();
	virtual void _Reserved8();
	
	status_t RemoteConnect(int32 producer, int32 consumer, int32 port);
	status_t RemoteDisconnect(int32 producer, int32 consumer);
	void RemoteConnected(int32 producer, int32 consumer);
	void RemoteDisconnected(int32 producer, int32 consumer);
	
	BMidiEndpoint* RemoteCreateProducer(int32 producer, const char *name);
	BMidiEndpoint* RemoteCreateConsumer(int32 consumer, const char *name, int32 port, int32 latency);
	void RemoteDelete(BMidiEndpoint *object);
	void RemoteRename(BMidiEndpoint *object, const char *name);
	void RemoteChangeLatency(BMidiEndpoint *object, bigtime_t latency);
	
	status_t Remote(int32 id, int32 op);
	status_t DoRemote(BMessage *msg, BMessage *result = 0);
	
	void Rename(BMidiEndpoint *midi, const char *name);
	status_t Release(BMidiEndpoint *midi);
	status_t Acquire(BMidiEndpoint *midi);
	
	void Create(BMidiEndpoint *midi);
	void SetLatency(BMidiConsumer *midi, bigtime_t latency);
	status_t SetProperties(BMidiEndpoint *midi, const BMessage *props);
	status_t GetProperties(const BMidiEndpoint *midi, BMessage *props);
	
	status_t Connect(BMidiProducer *source, BMidiConsumer *sink);	
	status_t Disconnect(BMidiProducer *source, BMidiConsumer *sink);
	
	BLooper *Looper();
	BMidiRosterLooper *looper;
	BMessenger *remote;
	
	uint32 _reserved[16];
};

#endif
