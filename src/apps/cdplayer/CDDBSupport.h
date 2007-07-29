#ifndef CDDBSUPPORT_H
#define CDDBSUPPORT_H

#include "CDAudioDevice.h"

#include <Entry.h>
#include <List.h>
#include <NetEndpoint.h>
#include <String.h>

#include <scsi.h>
#include <vector>

using std::vector;


class CDDBData {
	public:
		CDDBData(int32 discid = -1);
		CDDBData(const CDDBData &from);
		~CDDBData();
		CDDBData& operator=(const CDDBData &from);

		status_t Load(const entry_ref &ref);
		status_t Save(const char *filename);

		status_t Load();
		status_t Save();

		void MakeEmpty();

		void SetDiscID(const int32 &id)
		{
			fDiscID = id;
		}

		int32		DiscID() const
		{
			return fDiscID;
		}

		uint16	CountTracks() const;
		const char*	TrackAt(const int16 &index) const;
		bool RenameTrack(const int32 &index, const char *newname);
		void AddTrack(const char *track, const CDAudioTime &time, 
				const int16 &index=-1);
		void RemoveTrack(const int16 &index);

		CDAudioTime* TrackTimeAt(const int16 &index);

		void SetDiscTime(const CDAudioTime time)
		{
			fDiscTime = time;
		}

		CDAudioTime* DiscTime()
		{
			return &fDiscTime;
		}

		void SetGenre(const char *genre)
		{
			fGenre=genre;
		}

		const char*	Genre()
		{
			return fGenre.String();
		}

		void SetArtist(const char *artist)
		{
			fArtist = artist;
		}

		const char*	Artist() const
		{
			return fArtist.String();
		}

		void SetAlbum(const char *album)
		{
			fAlbum = album;
		}

		const char*	Album() const
		{
			return fAlbum.String();
		}

		void SetYear(const int16 &year)
		{
			fYear = year;
		}

		int16 Year() const
		{
			return fYear;
		}

	private:
		void		_EmptyLists();

		int32		fDiscID;

		BString		fArtist;
		BString		fAlbum;
		BString		fGenre;

		BList		fTrackList;
		BList		fTimeList;

		CDAudioTime	fDiscTime;
		int32		fYear;
};


// based on Jukebox by Chip Paul


class CDDBQuery {
	public:
		CDDBQuery(const char *server, int32 port = 888);
		~CDDBQuery(); 
		void SetToSite(const char *server, int32 port);
		status_t GetSites(bool (*)(const char *site, int port, 
			const char *latitude, const char *longitude,
			const char *description, void *state), void *);

		void SetToCD(const char *devicepath);

		const char*	GetArtist()
		{
			return fCDData.Artist();
		}

		const char*	GetAlbum()
		{
			return fCDData.Album();
		}

		const char*	GetGenre()
		{
			return fCDData.Genre();
		}

		bool GetData(CDDBData *data, bigtime_t timeout);
		void SetData(const CDDBData &data);

		bool Ready() const
		{
			return fState == kDone;
		}

		int32 CurrentDiscID() const
		{
			return fCDData.DiscID();
		}

		static int32 GetDiscID(const scsi_toc *);
		static int32 GetTrackCount(const scsi_toc *);
		static CDAudioTime GetDiscTime(const scsi_toc *);
		static void GetTrackTimes(const scsi_toc *, vector<CDAudioTime> &times);
		static BString OffsetsToString(const scsi_toc *);

	private:
		status_t _Connect();
		void _Disconnect();
		bool _IsConnected() const;

		static int32 _QueryThread(void *);

		// cached retrieved data
		enum State {
			kInitial,
			kReading,
			kDone,
			kInterrupting,
			kError
		};

		void _SetDefaultInfo();
		status_t _OpenContentFile(const int32 &discID);

		status_t _ReadFromServer(BString &data);
		void _ParseData(const BString &data);
		void _WriteFile();

		void _ReadLine(BString &);
		status_t _IdentifySelf();

		const char* _GetToken(const char*, BString &);

		// state data
		BString			fServerName;
		BNetEndpoint	fSocket;
		int32			fPort;
		bool			fConnected;

		thread_id		fThread;
		State			fState;
		status_t		fResult;
	
		// disc information
		CDDBData		fCDData;
		BString			fFrameOffsetString;
		scsi_toc		fSCSIData;
};

BString GetLineFromString(const char *string);

#endif	// CDDBSUPPORT_H
