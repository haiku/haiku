#ifndef CDDBSUPPORT_H
#define CDDBSUPPORT_H

#include <NetEndpoint.h>
#include <String.h>
#include <scsi.h>
#include <vector>
#include "CDAudioDevice.h"

// based on Jukebox by Chip Paul

class CDDBQuery 
{
public:
							CDDBQuery(const char *server, int32 port = 888, 
									  bool log = false);
							~CDDBQuery(void); 
			void			SetToSite(const char *server, int32 port);
			void			GetSites(bool (*)(const char *site, int port, 
									 const char *latitude, const char *longitude,
									 const char *description, void *state), void *);

			void			SetToCD(const char *devicepath);
			
			const char *	GetArtist(void) { return fArtist.String(); }
			const char *	GetAlbum(void) { return fTitle.String(); }
			const char *	GetGenre(void) { return fCategory.String(); }
			
			bool			GetTitles(BString *title, vector<BString> *tracks, 
							  		  bigtime_t timeout);
	
			bool			Ready() const { return fState == kDone; }
			int32			CurrentDiscID() const	{ return fDiscID; }

	static	int32			GetDiscID(const scsi_toc *);
	static	int32			GetTrackCount(const scsi_toc *);
	static	cdaudio_time	GetDiscTime(const scsi_toc *);
	static	void			GetTrackTimes(const scsi_toc *, vector<cdaudio_time> &times);
	static	BString			OffsetsToString(const scsi_toc *);
	
private:
			void					Connect();
			void					Disconnect();
			bool					IsConnected() const;
	
	static	int32	QueryThread(void *);

	class Connector 
	{
		public:
			Connector(CDDBQuery *client)
				:	client(client),
					wasConnected(client->IsConnected())
				{
					if (!wasConnected)
						client->Connect();
				}
			
			~Connector()
				{
					if (!wasConnected)
						client->Disconnect();
				}
				
		private:
			CDDBQuery *client;
			bool wasConnected;
	};
	friend class Connector;

	// cached retrieved data
	enum State 
	{
		kInitial,
		kReading,
		kDone,
		kInterrupting,
		kError
	};
	
			bool					OpenContentFile(const int32 &discID);
	
			void					ReadFromServer(BString &data);
			void					ParseData(const BString &data);
			void					WriteFile(void);
			
			BString					GetLineFromString(const char *string);
			void					ReadLine(BString &);
			void					IdentifySelf();

			const char *			GetToken(const char *, BString &);
	
			bool					fLog;
	
			// state data
			BString					fServerName;
			BNetEndpoint			fSocket;
			int32					fPort;
			bool					fConnected;
			
			thread_id				fThread;
			State					fState;
			status_t				fResult;
			
			// disc information
			BString					fArtist;
			BString					fTitle;
			BString					fCategory;
			
			int32					fDiscID;
			int32					fDiscLength;
			
			vector<BString>			fTrackNames;
			int32					fTrackCount;
			
			BString					fFrameOffsetString;
			vector<cdaudio_time>	fTrackTimes;
};

#endif
