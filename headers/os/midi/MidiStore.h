#ifndef _MIDI_STORE_H
#define _MIDI_STORE_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <Midi.h>

struct entry_ref;

class BMidiEvent;
class BFile;

/*------------------------------------------------------------*/

class BMidiStore : public BMidi {
public:
				BMidiStore();
virtual			~BMidiStore();

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

virtual	void	TempoChange(int32 bpm, uint32 time = B_NOW);

		status_t	Import(const entry_ref *ref);
		status_t	Export(const entry_ref *ref, int32 format);

		void	SortEvents(bool force=false);
		uint32	CountEvents() const;

		uint32	CurrentEvent() const;
		void	SetCurrentEvent(uint32 eventNumber);

		uint32	DeltaOfEvent(uint32 eventNumber) const;
		uint32	EventAtDelta(uint32 time) const;

		uint32	BeginTime() const;
	
		void	SetTempo(int32 bpm);
		int32	Tempo() const;

private:

virtual	void		_ReservedMidiStore1();
virtual	void		_ReservedMidiStore2();
virtual	void		_ReservedMidiStore3();

virtual	void	Run();

		void	AddEvent(uint32 time, 
						 bool inMS, 
						 uchar type, 
		   	         	 uchar data1 = 0, 
						 uchar data2 = 0,
		   	         	 uchar data3 = 0, 	 
						 uchar data4 = 0);

		void	AddSystemExclusive(void* data, size_t dataLength);
	
		status_t ReadHeader();
		bool	ReadMT(char*);
		int32	Read32Bit();
		int32	EGetC();
		int32	To32Bit(int32, int32, int32, int32);
		int32	Read16Bit();
		int32	To16Bit(int32, int32);
		bool	ReadTrack();
		int32	ReadVariNum();
		void	ChannelMessage(int32, int32, int32);
		void	MsgInit();
		void	MsgAdd(int32);
		void	BiggerMsg();
		void	MetaEvent(int32);
		int32	MsgLength();
		uchar*	Msg();
		void	BadByte(int32);
	
		int32	PutC(int32 c);
		bool	WriteTrack(int32 track);
		void	WriteTempoTrack();
		bool	WriteTrackChunk(int32 whichTrack);
		void	WriteHeaderChunk(int32 format);
		bool	WriteMidiEvent(uint32 deltaTime, 
							   uint32 type, 
							   uint32 channel, 
							   uchar* data, 
							   uint32 size);
		bool	WriteMetaEvent(uint32 deltaTime, 
							   uint32 type, 
							   uchar* data, 
							   uint32 size);
		bool	WriteSystemExclusiveEvent(uint32 deltaTime, 
							    uchar* data, 
							    uint32 size);
		void	WriteTempo(uint32 deltaTime, int32 tempo);
		void	WriteVarLen(uint32 value);
		void	Write32Bit(uint32 data);
		void	Write16Bit(ushort data);
		int32	EPutC(uchar c);
	
		uint32	TicksToMilliseconds(uint32 ticks) const;
		uint32	MillisecondsToTicks(uint32 ms) const;

		BList		*events;
//		uint32		fNumEvents;
//		uint32		fEventsSize;
		uint32		fCurrEvent;
		bool		fNeedsSorting;
//		bool		fResetTimer;
		uint32		fStartTime;
		BFile*		fFile;
		short		fDivision;
		float		fDivisionFactor;
		int32		fTempo;
		int32		fCurrTime;
		int32		fCurrTrack;
		int32		fNumTracks;
		
//		int32		fToBeRead;
//		int32		fMsgIndex;
//		int32		fMsgSize;
//		uchar*		fMsgBuff;
	
		int32		fNumBytesWritten;

		uchar*		fFileBuffer;
		int32		fFileBufferMax;
		int32		fFileBufferSize;
		int32		fFileBufferIndex;
		uint32		_reserved[4];
};


/*------------------------------------------------------------*/

#endif
