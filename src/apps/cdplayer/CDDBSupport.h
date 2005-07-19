#ifndef CDDBSUPPORT_H
#define CDDBSUPPORT_H

#include <NetEndpoint.h>
#include <String.h>
#include <scsi.h>
#include <vector>


// based on Jukebox by Chip Paul

class CDDBQuery 
{
public:
					CDDBQuery(const char *server, int32 port = 888, bool log = false);
			void	SetToSite(const char *server, int32 port);
			void	GetSites(bool (*)(const char *site, int port, const char *latitude,
									  const char *longitude, const char *description, 
									  void *state), void *);

			void	SetToCD(const char *devicepath);
			bool	GetTitles(BString *title, vector<BString> *tracks, 
							  bigtime_t timeout);
	
			bool	Ready() const { return fState == kDone; }
			int32	CurrentDiscID() const	{ return fDiscID; }

	static	int32	GetDiscID(const scsi_toc *);

private:
	static	void	GetDiscID(const scsi_toc *, int32 &id, int32 &numTracks, 
							  int32 &length, BString &tmpFrameOffsetString,
							  BString &discIDString);
			void	Connect();
			void	Disconnect();
			bool	IsConnected() const;
	
	static	int32	LookupBinder(void *);

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
	
			bool			FindOrCreateContentFileForDisk(BFile *file, entry_ref *ref,
														   int32 discID);
	
			void			ReadFromServer(BDataIO *);
			void			ParseResult(BDataIO *);

			void			ReadLine(BString &);
	static	void			ReadLine(BDataIO *, BString &);
			void			IdentifySelf();

			const char *	GetToken(const char *, BString &);
	
			bool			fLog;
	
			// connection description
			BString			fServerName;
			BNetEndpoint	fSocket;
			int32			fPort;
			bool			fConnected;
			
			// disc identification
			int32			fDiscID;
			int32			fDiscLength;
			int32			fTrackCount;
			BString			fFrameOffsetString;
			BString			fDiscIDStr;
			
			thread_id		fThread;
			State			fState;
			BString			fTitle;
			vector<BString>	fTrackNames;
			status_t		fResult;
			
};

#endif
