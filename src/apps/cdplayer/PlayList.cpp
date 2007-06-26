#include "PlayList.h"

#include <OS.h>

#include <stdlib.h>
#include <string.h>

//#define DEBUG_PLAYLIST

#ifdef DEBUG_PLAYLIST
#include <stdio.h>
#define STRACE(x) printf x
#else
#define STRACE(x) /* nothing */
#endif


PlayList::PlayList(int16 count, int16 start)
	:
	fTrackCount(count),
	fTrackIndex(0),
	fStartingTrack(start),
	fRandom(false),
	fLoop(false)
{
	STRACE(("PlayList(count=%d,start=%d)\n",count,start));

	srand(real_time_clock_usecs());

	if (fTrackCount < 0)
		fTrackCount = 0;
	else if (fTrackCount > 500)
		fTrackCount = 500;

	if (fStartingTrack >= fTrackCount)
		fStartingTrack = fTrackCount - 1;

	if (fStartingTrack < 1)
		fStartingTrack = 1;

	memset(fTrackList, -1, 500);

	Unrandomize();
}


void
PlayList::SetTrackCount(const int16 &count)
{
	fLocker.Lock();
	
	STRACE(("PlayList::SetTrackCount(%d)\n",count));
	
	if (count <= 0) {
		fTrackCount = 0;
		fTrackIndex = 0;
	}
	else if (count > 500)
		fTrackCount = 500;
	else
		fTrackCount = count;

	memset(fTrackList, -1, 500);
	SetShuffle(IsShuffled());

	fLocker.Unlock();
}


void
PlayList::SetStartingTrack(const int16 &start)
{
	fLocker.Lock();
	
	STRACE(("PlayList::SetStartingTrack(%d)\n",start));
	
	fStartingTrack = start;
	
	fLocker.Unlock();
}


void
PlayList::Rewind()
{
	STRACE(("PlayList::Rewind()\n"));
	fLocker.Lock();
	
	fTrackIndex = 0;
	
	fLocker.Unlock();
}


void
PlayList::SetShuffle(const bool &random)
{
	STRACE(("PlayList::SetShuffle(%s)\n", random ? "random" : "sequential"));
	fLocker.Lock();

	if (random)
		Randomize();
	else
		Unrandomize();
	
	fRandom = random;
	
	fLocker.Unlock();
}


void
PlayList::SetLoop(const bool &loop)
{
	STRACE(("PlayList::SetLoop(%s)\n", loop ? "loop" : "non-loop"));
	fLocker.Lock();

	fLoop = loop;

	fLocker.Unlock();
}

	
void
PlayList::SetCurrentTrack(const int16 &track)
{
	STRACE(("PlayList::SetCurrentTrack(%d)\n",track));
	
	if (track < 0 || track > fTrackCount)
		return;

	fLocker.Lock();
	
	for (int16 i = 0; i < fTrackCount; i++) {
		if (fTrackList[i] == track) {
			fTrackIndex = i;
			break;
		}
	}

	fLocker.Unlock();
}


int16
PlayList::GetCurrentTrack()
{
	fLocker.Lock();

	int16 value = fTrackList[fTrackIndex];
//	STRACE(("PlayList::GetCurrentTrack()=%d\n",value));

	fLocker.Unlock();
	return value;
}


int16
PlayList::GetNextTrack()
{
	fLocker.Lock();

	if (fTrackCount < 1) {
		fLocker.Unlock();
		STRACE(("PlayList::GetNextTrack()=-1 (no tracks)\n"));
		return -1;
	}

	if (fTrackIndex > (fTrackCount - fStartingTrack)) {
		if (fLoop)
			fTrackIndex = 0;
		else {
			fLocker.Unlock();
			STRACE(("PlayList::GetNextTrack()=-1 (track index out of range)\n"));
			return -1;
		}
	}
	else
		fTrackIndex++;
	
	int16 value = fTrackList[fTrackIndex];
	STRACE(("PlayList::GetNextTrack()=%d\n",value));
	
	fLocker.Unlock();
	return value;
}


int16
PlayList::GetPreviousTrack()
{
	fLocker.Lock();

	if (fTrackCount < 1) {
		fLocker.Unlock();
		STRACE(("PlayList::GetPreviousTrack()=-1 (no tracks)\n"));
		return -1;
	}

	if (fTrackIndex == 0) {
		if (fLoop)
			fTrackIndex = (fTrackCount - fStartingTrack);
		else {
			fLocker.Unlock();
			STRACE(("PlayList::GetPreviousTrack()=-1 (track index out of range)\n"));
			return -1;
		}
	}
	else
		fTrackIndex--;
	
	int16 value = fTrackList[fTrackIndex];
	STRACE(("PlayList::GetPreviousTrack()=%d\n",value));
	fLocker.Unlock();
	return value;
}


int16
PlayList::GetLastTrack()
{
	fLocker.Lock();

	if (fTrackCount < 1) {
		fLocker.Unlock();
		STRACE(("PlayList::GetLastTrack()=-1 (no tracks)\n"));
		return -1;
	}

	fTrackIndex = fTrackCount - 1;
	int16 value = fTrackList[fTrackIndex];
	STRACE(("PlayList::GetLastTrack()=%d\n",value));
	fLocker.Unlock();
	return value;
}


int16
PlayList::GetFirstTrack()
{
	fLocker.Lock();
	
	if (fTrackCount < 1) {
		fLocker.Unlock();
		STRACE(("PlayList::GetFirstTrack()=-1 (no tracks)\n"));
		return -1;
	}
	
	fTrackIndex = 0;
	int16 value = fTrackList[fTrackIndex];
	STRACE(("PlayList::GetFirstTrack()=%d\n",value));
	fLocker.Unlock();
	return value;
}


void
PlayList::Randomize()
{
	STRACE(("PlayList::Randomize()\n"));
	
	// Reinitialize the count
	for (int16 i = fStartingTrack; i <= fTrackCount; i++)
		fTrackList[i - fStartingTrack] = i;
	
	// There are probably *much* better ways to do this,
	// but this is the only one I could think of. :(
	
	int32 listcount = (fTrackCount - fStartingTrack);
	int32 swapcount =  listcount * 2;
	
	int16 temp, first, second;
	for (int32 i = 0; i < swapcount; i++) {
		// repeatedly pick two elements at random and swap them
		// This way we are sure to not have any duplicates and still have
		// all tracks eventually be played.
		
		first = (int16)(listcount * ((float)rand() / RAND_MAX));
		second = (int16)(listcount * ((float)rand() / RAND_MAX));

		temp = fTrackList[first];
		fTrackList[first] = fTrackList[second];
		fTrackList[second] = temp;
	}

	#ifdef DEBUG_PLAYLIST
		for (int16 i = fStartingTrack; i <= fTrackCount; i++)
			printf("\tSlot %d: track %d\n", i, fTrackList[i]);
	#endif
}


void
PlayList::Unrandomize()
{
	STRACE(("PlayList::Unrandomize()\n"));
	for (int16 i = fStartingTrack; i <= fTrackCount; i++)
		fTrackList[i - fStartingTrack] = i;
}
