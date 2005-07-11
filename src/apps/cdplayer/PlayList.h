#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <SupportDefs.h>
#include <Locker.h>

class PlayList
{
public:
			PlayList(int16 tracks = 0, int16 startingtrack = 1);
			
	void	SetTrackCount(const int16 &count);
	int16	TrackCount(void) const { return fTrackCount; }
	
	void	SetStartingTrack(const int16 &start);
	int16	StartingTrack(void) const { return fStartingTrack; }
	
	void	Rewind(void);
	
	void	SetShuffle(const bool &random);
	bool	IsShuffled(void) const { return fRandom; }
	
	void	SetLoop(const bool &loop);
	bool	IsLoop(void) const { return fLoop; }
	
	void	SetCurrentTrack(const int16 &track);
	int16	GetCurrentTrack(void);
	
	int16	GetNextTrack(void);
	int16	GetPreviousTrack(void);
	
private:
	void	Randomize(void);
	void	Unrandomize(void);
	
	int16	fTrackCount;
	int16	fTrackIndex;
	int16	fStartingTrack;
	int16	fTrackList[500];	// This should be big enough. :)
	
	bool	fRandom;
	bool	fLoop;
	
	BLocker	fLocker;
};

#endif
