/**
 * @file MidiPort.h
 * 
 * @author Jerome Leveque
 */

//----------------------------------------

#ifndef _MIDI_PORT_CONSUMER_H
#define _MIDI_PORT_CONSUMER_H

#include <MidiConsumer.h>
#include <MidiPort.h>

//----------------------------------------

class BMidiPortConsumer : public BMidiLocalConsumer
{
public:
	BMidiPortConsumer(BMidiPort *toPort, const char *name = NULL);
	
	void NoteOff(uchar channel, uchar note, uchar velocity, bigtime_t time);
	void NoteOn(uchar channel, uchar note, uchar velocity, bigtime_t time);
	void KeyPressure(uchar channel, uchar note, uchar pressure, bigtime_t time);
	void ControlChange(uchar channel, uchar controlNumber,
						uchar controlValue, bigtime_t time);
	void ProgramChange(uchar channel, uchar programNumber, bigtime_t time);
	void ChannelPressure(uchar channel, uchar pressure, bigtime_t time);
	void PitchBend(uchar channel, uchar lsb, uchar msb, bigtime_t time);
	void SystemExclusive(void *data, size_t dataLength, bigtime_t time);
	void SystemCommon(uchar statusByte, uchar data1,
						uchar data2, bigtime_t time);
	void SystemRealTime(uchar statusByte, bigtime_t time);

private:
	BMidiPort *port;

};

//----------------------------------------

#endif _MIDI_PORT_CONSUMER_H
