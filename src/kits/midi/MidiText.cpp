//-----------------------------------------------------------------------------
//
#include <MidiText.h>
#include <iostream.h>
#include "MidiEvent.h"

BMidiText::BMidiText() {
	_start_time = 0x0;
}

BMidiText::~BMidiText() {
}

void BMidiText::NoteOff(uchar chan, uchar note, uchar vel, uint32 time) {
	SnoozeUntil(time);
	_PrintTime();
	cout << ":NOTE OFF;channel = " << (int)chan
		<< ",note = " << (int)note << ",velocity = " << (int)vel << endl;
}

void BMidiText::NoteOn(uchar chan, uchar note, uchar vel, uint32 time) {
	SnoozeUntil(time);
	_PrintTime();
	cout << ":NOTE ON;channel = " << (int)chan
		<< ",note = " << (int)note << ",velocity = " << (int)vel << endl;
}

void BMidiText::KeyPressure(uchar chan, uchar note, uchar pres, uint32 time) {
	SnoozeUntil(time);
	_PrintTime();
	cout << ":KEY PRESSURE;channel = " << (int)chan
		<< ",note = " << (int)note << ",pressure = " << (int)pres << endl;
}

void BMidiText::ControlChange(uchar chan, uchar ctrl_num,
                               uchar ctrl_val, uint32 time) {
	SnoozeUntil(time);
	_PrintTime();
	cout << ":CONTROL CHANGE;channel = " << (int)chan
		<< ",control = " << (int)ctrl_num
		<< ",value = "<< (int)ctrl_val << endl;
}

void BMidiText::ProgramChange(uchar chan, uchar prog_num, uint32 time) {
	SnoozeUntil(time);
	_PrintTime();
	cout << ":PROGRAM CHANGE;channel = " << (int)chan
		<< ",program = " << (int)prog_num << endl;
}

void BMidiText::ChannelPressure(uchar chan, uchar pres, uint32 time) {
	SnoozeUntil(time);
	_PrintTime();
	cout << ":CHANNEL PRESSURE;channel = " << (int)chan
		<< ",pressure = " << (int)pres << endl;
}

void BMidiText::PitchBend(uchar chan, uchar lsb, uchar msb, uint32 time) {
	SnoozeUntil(time);
	_PrintTime();
	cout << ":PITCH BEND;channel = " << (int)chan << hex
		<< ",lsb = " << (int)lsb << ",msb = " << (int)msb << endl;
}

void BMidiText::SystemExclusive(void * data, size_t data_len, uint32 time) {
	SnoozeUntil(time);
	_PrintTime();
	cout << ":SYSTEM EXCLUSIVE;";
	for(size_t i = 0; i < data_len; i++) {
		cout << hex << (int)((char *)data)[i];
	}
	cout << endl;
}

void BMidiText::SystemCommon(uchar stat_byte, uchar data1,
	uchar data2, uint32 time) {
	SnoozeUntil(time);
	_PrintTime();
	cout << ":SYSTEM COMMON;status = " << hex << (int)stat_byte
		<< ",data1 = " << (int)data1 << ",data2 = " << (int)data2 << endl;                              
}

void BMidiText::SystemRealTime(uchar stat_byte, uint32 time) {
	SnoozeUntil(time);
	_PrintTime();
	cout << ":SYSTEM REAL TIME;status = " << hex << (int)stat << endl;
}

void BMidiText::TempoChange(int32 bpm, uint32 time) {
	SnoozeUntil(time);
	_PrintTime();
	cout << ":TEMPO CHANGE;" << "beatsperminute = " << bpm << endl;
}

void BMidiText::AllNotesOff(bool just_chan, uint32 time) {
	SnoozeUntil(time);
	_PrintTime();
	cout << ":ALL NOTES OFF;" << endl;
}

void BMidiText::ResetTimer(bool start) {
	if(start) {
		_start_time = (uint32)(system_time() / 1000);
	} else {
		_start_time = 0;
	}
}

//-----------------------------------------------------------------------------
//
void BMidiText::_PrintTime() {
	uint32 cur_time = B_NOW;
	if(_start_time == 0) {
		_start_time = cur_time;	
	}
	cout << (cur_time - _start_time);	
}
