#ifndef __CDDBSUPPORT__
#define __CDDBSUPPORT__

#include <NetEndpoint.h>
#include <String.h>
#include <scsi.h>
#include <vector>


// based on Jukebox by Chip Paul

class CDDBQuery 
{
public:
	CDDBQuery(const char *server, int32 port = 888, bool log = false);
	void SetToSite(const char *server, int32 port);
	void GetSites(bool (*)(const char *site, int port, const char *latitude,
		const char *longitude, const char *description, void *state), void *);

	void SetToCD(const char *devicepath);
	bool GetTitles(BString *title, vector<BString> *tracks, bigtime_t timeout);
		// title or tracks may be NULL if you are only interrested in one, not the other
	
	bool Ready() const
		{ return state == kDone; }

	int32 CurrentDiscID() const
		{ return discID; }

	static int32 GetDiscID(const scsi_toc *);

private:

	static void GetDiscID(const scsi_toc *, int32 &id, int32 &numTracks, int32 &length,
		BString &tmpFrameOffsetString, BString &discIDString);

	void Connect();
	void Disconnect();
	bool IsConnected() const;
	
	static int32 LookupBinder(void *);

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
	
	bool FindOrCreateContentFileForDisk(BFile *file, entry_ref *ref, int32 discID);
	
	void ReadFromServer(BDataIO *);
	void ParseResult(BDataIO *);

	void ReadLine(BString &);
	static void ReadLine(BDataIO *, BString &);
	void IdentifySelf();

	const char *GetToken(const char *, BString &);
	
	bool log;
	
	// connection description
	BString serverName;
	BNetEndpoint socket;
	int32 port;
	bool connected;
	
	// disc identification
	int32 discID;
	int32 discLength;
	int32 numTracks;
	BString frameOffsetString;
	BString discIDStr;

	// cached retrieved data
	enum State 
	{
		kInitial,
		kReading,
		kDone,
		kInterrupting,
		kError
	};
	
	thread_id thread;
	State state;
	BString title;
	vector<BString> trackNames;
	status_t result;

	friend class Connector;
};

#endif
