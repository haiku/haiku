#include "PlayList.h"
#include <OS.h>
#include <stdlib.h>
#include <string.h>

PlayList::PlayList(int16 tracks)
 : 	fTrackCount(0),
 	fTrackIndex(0),
 	fRandom(false),
 	fLoop(false)
{
	srand(real_time_clock_usecs());
	
	SetTrackCount(tracks);
}

void
PlayList::SetTrackCount(const int16 &count)
{
	if(count < 0)
		fTrackCount = 0;
	else
	if(count > 500)
		fTrackCount = 500;
	else
		fTrackCount = count;
	
	memset(fTrackList,-1,500);
	SetShuffle(IsShuffled());
}

void
PlayList::Rewind(void)
{
	fTrackIndex = 0;
}

void
PlayList::SetShuffle(const bool &random)
{
	if(random)
		Randomize();
	else
		Unrandomize();
}

void
PlayList::SetLoop(const bool &loop)
{
	fLoop = loop;
}
	
int16
PlayList::GetNextTrack(void)
{
	if(fTrackCount < 1)
		return -1;
	
	if(fTrackIndex == fTrackCount)
	{
		if(fLoop)
			fTrackIndex = 0;
		else
			return -1;
	}
	
	return fTrackList[fTrackIndex++];
}

void
PlayList::Randomize(void)
{
	// Reinitialize the count
	for(int16 i=0; i<fTrackCount; i++)
		fTrackList[i] = i;
	
	// There are probably *much* better ways to do this,
	// but this is the only one I could think of. :(
	
	int32 swapcount = fTrackCount * 2;
	
	int16 temp, first, second;
	for(int32 i=0; i< swapcount; i++)
	{
		// repeatedly pick two elements at random and swap them
		// This way we are sure to not have any duplicates and still have
		// all tracks eventually be played.
		
		first = (int16)(fTrackCount * ((float)rand()/RAND_MAX));
		second = (int16)(fTrackCount * ((float)rand()/RAND_MAX));
		
		temp = fTrackList[first];
		fTrackList[first] = fTrackList[second];
		fTrackList[second] = temp;
	}
}

void
PlayList::Unrandomize(void)
{
	for(int16 i=0; i<fTrackCount; i++)
		fTrackList[i] = i;
}

