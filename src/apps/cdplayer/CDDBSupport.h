#ifndef CDDBSUPPORT_H
#define CDDBSUPPORT_H

#include <NetEndpoint.h>
#include <String.h>
#include <scsi.h>
#include <vector>
#include <Entry.h>
#include <List.h>
#include "CDAudioDevice.h"

using std::vector;

class CDDBData
{
public:
							CDDBData(int32 discid=-1);
							CDDBData(const CDDBData &from);
							~CDDBData(void);
			CDDBData &		operator=(const CDDBData &from);
			
			status_t		Load(const entry_ref &ref);
			status_t		Save(const char *filename);
			
			status_t		Load(void);
			status_t		Save(void);
			
			void			MakeEmpty(void);
			void			SetDiscID(const int32 &id) { fDiscID=id; }
			int32			DiscID(void) const { return fDiscID; }
			
			uint16			CountTracks(void) const;
			const char *	TrackAt(const int16 &index) const;
			bool			RenameTrack(const int32 &index, const char *newname);
			void			AddTrack(const char *track, const cdaudio_time &time, 
									const int16 &index=-1);
			void			RemoveTrack(const int16 &index);
			
			cdaudio_time *	TrackTimeAt(const int16 &index);
			
			void			SetDiscTime(const cdaudio_time time) { fDiscTime = time; }
			cdaudio_time *	DiscTime(void) { return &fDiscTime; }
			
			void			SetGenre(const char *genre) { fGenre=genre; }
			const char *	Genre(void) { return fGenre.String(); }
			
			void			SetArtist(const char *artist) { fArtist=artist; }
			const char *	Artist(void) const { return fArtist.String(); }
			
			void			SetAlbum(const char *album) { fAlbum=album; }
			const char *	Album(void) const { return fAlbum.String(); }
			
			void			SetYear(const int16 &year) { fYear=year; }
			int16			Year(void) const { return fYear; }

			
private:
			void			EmptyLists(void);
			
			int32			fDiscID;
			
			BString			fArtist;
			BString			fAlbum;
			BString			fGenre;
			
			BList			fTrackList;
			BList			fTimeList;
			
			cdaudio_time	fDiscTime;
			int32			fYear;
};

// based on Jukebox by Chip Paul

class CDDBQuery 
{
public:
							CDDBQuery(const char *server, int32 port = 888);
							~CDDBQuery(void); 
			void			SetToSite(const char *server, int32 port);
			status_t		GetSites(bool (*)(const char *site, int port, 
									 const char *latitude, const char *longitude,
									 const char *description, void *state), void *);

			void			SetToCD(const char *devicepath);
			
			const char *	GetArtist(void) { return fCDData.Artist(); }
			const char *	GetAlbum(void) { return fCDData.Album(); }
			const char *	GetGenre(void) { return fCDData.Genre(); }
			
			bool			GetData(CDDBData *data, bigtime_t timeout);
			void			SetData(const CDDBData &data);
			
			bool			Ready() const { return fState == kDone; }
			int32			CurrentDiscID() const	{ return fCDData.DiscID(); }

	static	int32			GetDiscID(const scsi_toc *);
	static	int32			GetTrackCount(const scsi_toc *);
	static	cdaudio_time	GetDiscTime(const scsi_toc *);
	static	void			GetTrackTimes(const scsi_toc *, vector<cdaudio_time> &times);
	static	BString			OffsetsToString(const scsi_toc *);
	
private:
			status_t				Connect();
			void					Disconnect();
			bool					IsConnected() const;
	
	static	int32	QueryThread(void *);

	// cached retrieved data
	enum State 
	{
		kInitial,
		kReading,
		kDone,
		kInterrupting,
		kError
	};
			void					SetDefaultInfo(void);
			status_t				OpenContentFile(const int32 &discID);
	
			status_t				ReadFromServer(BString &data);
			void					ParseData(const BString &data);
			void					WriteFile(void);
			
			void					ReadLine(BString &);
			status_t				IdentifySelf();

			const char *			GetToken(const char *, BString &);
	
			// state data
			BString					fServerName;
			BNetEndpoint			fSocket;
			int32					fPort;
			bool					fConnected;
			
			thread_id				fThread;
			State					fState;
			status_t				fResult;
			
			// disc information
			CDDBData				fCDData;
			BString					fFrameOffsetString;
			scsi_toc				fSCSIData;
};

BString GetLineFromString(const char *string);

#endif
