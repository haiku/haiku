/**
 * @file MidiPort.cpp
 * 
 * @author Jerome Leveque
 */

//----------------------------------------

#include <MidiConsumer.h>
#include <MidiPort.h>

//----------------------------------------

class BMidiPortConsumer : public BMidiLocalConsumer
{
public:
	BMidiPortConsumer(BMidiPort *toPort, const char *name = NULL)
		:  BMidiLocalConsumer(name), port(toPort) {}
	
	void NoteOff(uchar channel, uchar note, uchar velocity, bigtime_t time)
	{
		port->SprayNoteOff(channel, note, velocity, B_NOW);
	}
	
	void NoteOn(uchar channel, uchar note, uchar velocity, bigtime_t time)
	{
		port->SprayNoteOn(channel, note, velocity, B_NOW);
	}
	
	void KeyPressure(uchar channel, uchar note, uchar pressure, bigtime_t time)
	{
		port->SprayKeyPressure(channel, note, pressure, B_NOW);
	}
	
	void ControlChange(uchar channel, uchar controlNumber, uchar controlValue, bigtime_t time)
	{
		port->SprayControlChange(channel, controlNumber, controlValue, B_NOW);
	}
	
	void ProgramChange(uchar channel, uchar programNumber, bigtime_t time)
	{
		port->SprayProgramChange(channel, programNumber, B_NOW);
	}
	
	void ChannelPressure(uchar channel, uchar pressure, bigtime_t time)
	{
		port->SprayChannelPressure(channel, pressure, B_NOW);
	}
	
	void PitchBend(uchar channel, uchar lsb, uchar msb, bigtime_t time)
	{
		port->SprayPitchBend(channel, lsb, msb, B_NOW);
	}
	
	void SystemExclusive(void *data, size_t dataLength, bigtime_t time)
	{
		port->SpraySystemExclusive(data, dataLength, B_NOW);
	}
	
	void SystemCommon(uchar statusByte, uchar data1, uchar data2, bigtime_t time)
	{
		port->SpraySystemCommon(statusByte, data1, data2, B_NOW);
	}
	
	void SystemRealTime(uchar statusByte, bigtime_t time)
	{
		port->SpraySystemRealTime(statusByte, B_NOW);
	}

private:
	BMidiPort *port;

};

