#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <SupportDefs.h>

class PlayList
{
public:
			PlayList(int16 tracks = 0);
			
	void	SetTrackCount(const int16 &count);
	int16	TrackCount(void) const { return fTrackCount; }
	
	void	Rewind(void);
	
	void	SetShuffle(const bool &random);
	bool	IsShuffled(void) const { return fRandom; }
	
	void	SetLoop(const bool &loop);
	bool	IsLoop(void) const { return fLoop; }
	
	int16	GetNextTrack(void);
	
private:
	void	Randomize(void);
	void	Unrandomize(void);
	
	int16	fTrackCount;
	int16	fTrackIndex;
	int16	fTrackList[500];	// This should be big enough. :)
	
	bool	fRandom;
	bool	fLoop;
	
	
};

#endif
