#include <stdio.h>
#include <unistd.h>

#include "PortDrivers.h"

//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

MidiPortConsumer::MidiPortConsumer(int filedescriptor, const char *name = NULL)
		: BMidiLocalConsumer(name),
		fd(filedescriptor)
{
};
	
//----------------------------

void MidiPortConsumer::Data(uchar *data, size_t length, bool atomic, bigtime_t time)
{
	snooze_until(time - Latency(), B_SYSTEM_TIMEBASE);
	if (write(fd, data, length) == -1)
		perror("Error while sending data to Hardware ");
};
	
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------

MidiPortProducer::MidiPortProducer(int filedescriptor, const char *name = NULL)
		: BMidiLocalProducer(name),
		fd(filedescriptor),
		KeepRunning(true)
{
	thread_id thread = spawn_thread(SpawnThread, "MidiDriversProducer", B_URGENT_PRIORITY, this);
	if (thread >= 0)
		resume_thread(thread);
};
	
//----------------------------	

MidiPortProducer::~MidiPortProducer(void)
{
	KeepRunning = false;
}

//----------------------------	

int32 MidiPortProducer::SpawnThread(MidiPortProducer *producer)
{
	return producer->GetData();
}

//----------------------------	

int32 MidiPortProducer::GetData(void)
{
uchar newdata[3];
uchar *message;
	while (KeepRunning)
	{
		if (read(fd, newdata, 1) != 1)
		{
			perror("Error while getting data ");
			return B_ERROR;
		}
		switch(newdata[0] & 0xF0)
		{
			case 0x80 :	read(fd, newdata + 1, 2);
						printf("Note OFF\n");
						SprayData(newdata, 3);
						break;
			case 0x90 : read(fd, newdata + 1, 2);
						printf("Note ON\n");
						SprayData(newdata, 3);
						break;
			case 0xA0 : read(fd, newdata + 1, 2);
						printf("AfterTouch\n");
						SprayData(newdata, 3);
						break;
			case 0xB0 : read(fd, newdata + 1, 2);
						printf("Controler\n");
						SprayData(newdata, 3);
						break;
			case 0xC0 : read(fd, newdata + 1, 1);
						printf("Program Change\n");
						SprayData(newdata, 2);
						break;
			case 0xD0 : read(fd, newdata + 1, 1);
						printf("Channel Pressure\n");
						SprayData(newdata, 2);
						break;
			case 0xE0 : read(fd, newdata + 1, 2);
						printf("Pitch Wheel\n");
						SprayData(newdata, 3);
						break;
		}
		switch (newdata[0])
		{
			case 0xF0 :	{
						int32 index = 0;
							message = new uchar[1000];
							read(fd, newdata, 1);
							while (newdata[0] != 0xF7)
							{
								message[index++] = newdata[0];
								read(fd, newdata, 1);
							}
							printf("System Exclusive\n");
							SpraySystemExclusive(message, index);
							//Must we delete message after passing it to SpraySystemExclusive??
							delete message;
						}
						break;
			case 0xF1 :	read(fd, newdata + 1, 1); //TimeCode Value
						printf("Quarter Frame Message\n");
						SprayData(newdata, 2);
						break;
			case 0xF2 :	read(fd, newdata + 1, 2); //LSB, MSB
						printf("Song Position Pointer\n");
						SprayData(newdata, 3);
						break;
			case 0xF3 :	read(fd, newdata + 1, 1); //Number
						printf("Song Selection\n");
						SprayData(newdata, 2);
						break;
			case 0xF6 :	printf("Tune Request\n");
						SprayData(newdata, 1);
						break;
			case 0xF8 :	printf("Timing Clock\n");
						SprayData(newdata, 1);
						break;
			case 0xFA :	printf("Start\n");
						SprayData(newdata, 1);
						break;
			case 0xFB :	printf("Continue\n");
						SprayData(newdata, 1);
						break;
			case 0xFC :	printf("Stop\n");
						SprayData(newdata, 1);
						break;
			case 0xFE :	printf("Active Sensing\n");
						SprayData(newdata, 1);
						break;
			case 0xFF :	read(fd, newdata + 1, 2); //Type, Size
						message = new uchar[newdata[2] + 3];
						message[0] = 0xFF;
						message[1] = newdata[1];
						message[2] = newdata[2];
						read(fd, message + 3, message[2]);
						printf("System Message\n");
						SprayData(message, message[1] + 3);
						//Must we delete message after passing it to SprayData??
						delete message;
						break;
		}
	}
return B_OK;
}
	
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
//----------------------------------------------------------
