
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

namespace BPrivate
{
	class BMidiRosterLooper;
	struct BMidiRosterKiller;
}

class BMidiRoster
{
public:

	static BMidiEndpoint *NextEndpoint(int32 *id);
	static BMidiProducer *NextProducer(int32 *id);
	static BMidiConsumer *NextConsumer(int32 *id);
	
	static BMidiEndpoint *FindEndpoint(int32 id, bool localOnly = false);
	static BMidiProducer *FindProducer(int32 id, bool localOnly = false);
	static BMidiConsumer *FindConsumer(int32 id, bool localOnly = false);
	
	static void StartWatching(const BMessenger *msngr);
	static void StopWatching();

	static status_t Register(BMidiEndpoint *endp);
	static status_t Unregister(BMidiEndpoint *endp);

	static BMidiRoster *MidiRoster();
	
private:

	friend class BMidiConsumer;
	friend class BMidiEndpoint;
	friend class BMidiLocalProducer;
	friend class BMidiLocalConsumer;
	friend class BMidiProducer;
	friend class BPrivate::BMidiRosterLooper;
	friend struct BPrivate::BMidiRosterKiller;

	BMidiRoster();
	virtual ~BMidiRoster();

	virtual void _Reserved1();
	virtual void _Reserved2();
	virtual void _Reserved3();
	virtual void _Reserved4();
	virtual void _Reserved5();
	virtual void _Reserved6();
	virtual void _Reserved7();
	virtual void _Reserved8();
	
	void CreateLocal(BMidiEndpoint*);
	void DeleteLocal(BMidiEndpoint*);

	status_t SendRequest(BMessage*, BMessage*);

	BPrivate::BMidiRosterLooper* fLooper;
	BMessenger *fServer;

	uint32 _reserved[16];
};

#endif // _MIDI_ROSTER_H
