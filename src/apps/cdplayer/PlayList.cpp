#include "PlayList.h"
#include <OS.h>
#include <stdlib.h>
#include <string.h>

PlayList::PlayList(int16 count, int16 start)
 : 	fTrackCount(count),
 	fTrackIndex(0),
 	fStartingTrack(start),
 	fRandom(false),
 	fLoop(false)
{
	srand(real_time_clock_usecs());
	
	if(fTrackCount < 0)
		fTrackCount = 0;
	else
	if(fTrackCount > 500)
		fTrackCount = 500;
	
	if(fStartingTrack >= fTrackCount)
		fStartingTrack = fTrackCount - 1;
	
	if(fStartingTrack < 1)
		fStartingTrack = 1;
	
	memset(fTrackList,-1,500);
	Unrandomize();
}

void
PlayList::SetTrackCount(const int16 &count)
{
	fLocker.Lock();
	
	if(count < 0)
		fTrackCount = 0;
	else
	if(count > 500)
		fTrackCount = 500;
	else
		fTrackCount = count;
	
	memset(fTrackList,-1,500);
	SetShuffle(IsShuffled());
	
	fLocker.Unlock();
}

void
PlayList::SetStartingTrack(const int16 &start)
{
	fLocker.Lock();
	
	if(start >= TrackCount())
		fStartingTrack = TrackCount() - 1;
	else
	if(start < 1)
		fStartingTrack = 1;
	else
		fStartingTrack = start;
	
	fLocker.Unlock();
}

void
PlayList::Rewind(void)
{
	fLocker.Lock();
	
	fTrackIndex = 0;
	
	fLocker.Unlock();
}

void
PlayList::SetShuffle(const bool &random)
{
	fLocker.Lock();
	
	if(random)
		Randomize();
	else
		Unrandomize();
	
	fLocker.Unlock();
}

void
PlayList::SetLoop(const bool &loop)
{
	fLocker.Lock();
	
	fLoop = loop;
	
	fLocker.Unlock();
}
	
int16
PlayList::GetCurrentTrack(void)
{
	fLocker.Lock();
	
	int16 value = fTrackList[fTrackIndex];
	
	fLocker.Unlock();
	return value;
}

int16
PlayList::GetNextTrack(void)
{
	fLocker.Lock();
	
	if(fTrackCount < 1)
	{
		fLocker.Unlock();
		return -1;
	}
	
	if(fTrackIndex > (fTrackCount - fStartingTrack))
	{
		if(fLoop)
			fTrackIndex = 0;
		else
		{
			fLocker.Unlock();
			return -1;
		}
	}
	else
		fTrackIndex++;
	
	int16 value = fTrackList[fTrackIndex];
	fLocker.Unlock();
	return value;
}

int16
PlayList::GetPreviousTrack(void)
{
	fLocker.Lock();
	
	if(fTrackCount < 1)
	{
		fLocker.Unlock();
		return -1;
	}
	
	if(fTrackIndex == 0)
	{
		if(fLoop)
			fTrackIndex = (fTrackCount - fStartingTrack);
		else
		{
			fLocker.Unlock();
			return -1;
		}
	}
	else
		fTrackIndex--;
	
	int16 value = fTrackList[fTrackIndex];
	fLocker.Unlock();
	return value;
}

void
PlayList::Randomize(void)
{
	// Reinitialize the count
	for(int16 i=fStartingTrack; i<=fTrackCount; i++)
		fTrackList[i - fStartingTrack] = i;
	
	// There are probably *much* better ways to do this,
	// but this is the only one I could think of. :(
	
	int32 listcount = (fTrackCount - fStartingTrack);
	int32 swapcount =  listcount* 2;
	
	int16 temp, first, second;
	for(int32 i=0; i< swapcount; i++)
	{
		// repeatedly pick two elements at random and swap them
		// This way we are sure to not have any duplicates and still have
		// all tracks eventually be played.
		
		first = (int16)(listcount * ((float)rand()/RAND_MAX));
		second = (int16)(listcount * ((float)rand()/RAND_MAX));
		
		temp = fTrackList[first];
		fTrackList[first] = fTrackList[second];
		fTrackList[second] = temp;
	}
}

void
PlayList::Unrandomize(void)
{
	for(int16 i=fStartingTrack; i<=fTrackCount; i++)
		fTrackList[i - fStartingTrack] = i;
}

