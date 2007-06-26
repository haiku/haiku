#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <Locker.h>
#include <SupportDefs.h>

class PlayList {
	public:
		PlayList(int16 tracks = 0, int16 startingtrack = 1);
	
		void SetTrackCount(const int16 &count);
		int16 TrackCount() const { return fTrackCount; }

		void SetStartingTrack(const int16 &start);
		int16 StartingTrack() const { return fStartingTrack; }

		void Rewind();

		void SetShuffle(const bool &random);
		bool IsShuffled() const { return fRandom; }

		void SetLoop(const bool &loop);
		bool IsLoop() const { return fLoop; }
	
		void SetCurrentTrack(const int16 &track);
		int16 GetCurrentTrack();
	
		int16 GetNextTrack();
		int16 GetPreviousTrack();

		int16 GetFirstTrack();
		int16 GetLastTrack();

	private:
		void Randomize();
		void Unrandomize();

		int16 fTrackCount;
		int16 fTrackIndex;
		int16 fStartingTrack;
		int16 fTrackList[500];	// This should be big enough. :)

		bool fRandom;
		bool fLoop;

		BLocker	fLocker;
};

#endif	// PLAYLIST_H
