/*******************************************************************************
/
/	File:		MidiPort.h
/
/	Description:	Interface to MIDI hardware ports.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _MIDI_PORT_H
#define _MIDI_PORT_H

#include <Midi.h>

class BMidiLocalProducer;
class BMidiPortConsumer;

/*------------------------------------------------------------*/

class BMidiPort : public BMidi {

public:
				BMidiPort(const char *name=NULL);
				~BMidiPort();

		status_t	InitCheck() const; 
		status_t	Open(const char *name);
		void	Close();

		const char * PortName() const;

virtual	void	NoteOff(uchar channel, 
						uchar note, 
						uchar velocity,
						uint32 time = B_NOW);

virtual	void	NoteOn(uchar channel, 
					   uchar note, 
					   uchar velocity,
			    	   uint32 time = B_NOW);

virtual	void	KeyPressure(uchar channel, 
							uchar note, 
							uchar pressure,
							uint32 time = B_NOW);

virtual	void	ControlChange(uchar channel, 
							  uchar controlNumber,
							  uchar controlValue, 
							  uint32 time = B_NOW);

virtual	void	ProgramChange(uchar channel, 
								uchar programNumber,
							  	uint32 time = B_NOW);

virtual	void	ChannelPressure(uchar channel, 
								uchar pressure, 
								uint32 time = B_NOW);

virtual	void	PitchBend(uchar channel, 
						  uchar lsb, 
						  uchar msb,
			    		  uint32 time = B_NOW);

virtual	void	SystemExclusive(void* data, 
								size_t dataLength, 
								uint32 time = B_NOW);

virtual	void	SystemCommon(uchar statusByte, 
							 uchar data1, 
							 uchar data2,
							 uint32 time = B_NOW);

virtual	void	SystemRealTime(uchar statusByte, uint32 time = B_NOW);

virtual	status_t	Start();
virtual	void	Stop();

        int32       CountDevices();
        status_t    GetDeviceName(int32 n, char * name,
	                        size_t bufSize = B_OS_NAME_LENGTH);


private:
typedef BMidi _inherited;

virtual	void		_ReservedMidiPort1();
virtual	void		_ReservedMidiPort2();
virtual	void		_ReservedMidiPort3();

virtual	void	Run();
friend class BMidiPortConsumer;

		void Dispatch(const unsigned char * buffer, size_t size, bigtime_t when);
		ssize_t	Read(void *buffer, size_t numBytes) const;
		ssize_t	Write(void *buffer, size_t numBytes, uint32 time) const;

        void        ScanDevices();

		BMidiProducer *remote_source;
		BMidiConsumer *remote_sink;
		
		char*	fName;
		status_t fCStatus;
		BList *	_fDevices;
		uint8 _m_prev_cmd;
		bool _m_enhanced;
		uint8 _m_reserved[2];
		
		// used to glue us to the new midi kit
		BMidiLocalProducer *local_source;
		BMidiPortConsumer *local_sink;
};

/*------------------------------------------------------------*/

#endif
