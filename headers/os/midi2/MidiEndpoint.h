#ifndef _MIDI_ENDPOINT_H
#define _MIDI_ENDPOINT_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <Midi2Defs.h>
#include <OS.h>

#include <String.h>

class BMidiList;
class BList;

class BMidiProducer;
class BMidiConsumer;

class BMidiList;

// dynamic_cast<BMidiProducer*> (ptr_to_endpoint)

class BMidiEndpoint 
{
public:
	const char *Name() const;
	void SetName(const char *name);
	
	int32 ID() const;

	bool IsProducer() const;
	bool IsConsumer() const;
	bool IsRemote() const;
	bool IsLocal() const;
	bool IsPersistent() const;
	bool IsValid() const;

	status_t Release();
	status_t Acquire();

	status_t SetProperties(const BMessage *props);
	status_t GetProperties(BMessage *props) const;

	status_t Register(void);
	status_t Unregister(void);
						
private:
	friend class BMidiRoster;
	friend class BMidiProducer;
	friend class BMidiConsumer;
	friend class BMidiLocalProducer;
	friend class BMidiLocalConsumer;
	friend class BMidiList;
	
	BMidiEndpoint(const char *name);
	virtual	~BMidiEndpoint();
	BMidiEndpoint(const BMidiEndpoint &) {}
	BMidiEndpoint& operator=(const BMidiEndpoint &) { return *this; }
	
	virtual void _Reserved1();
	virtual void _Reserved2();
	virtual void _Reserved3();
	virtual void _Reserved4();
	virtual void _Reserved5();
	virtual void _Reserved6();
	virtual void _Reserved7();
	virtual void _Reserved8();
	
	int32 fID;
	BString fName;	
	status_t fStatus;

	int32 fFlags;
	int32 fRefCount;
		
	uint32 _reserved[4];
};

#endif
