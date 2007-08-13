#ifndef __SERVER_PICTURE_H
#define __SERVER_PICTURE_H

#include <DataIO.h>

#include <PictureDataWriter.h>

#include <PortLink.h> 
	// TODO: For some reason, the forward declaration "class BPrivate::PortLink" causes compiling errors 


class ServerApp;
class ViewLayer;
class BPrivate::LinkReceiver;
class BList;
class ServerPicture : public PictureDataWriter {
public:	
		int32		Token() { return fToken; }
		
		void		EnterStateChange();
		void		ExitStateChange();

		void		EnterFontChange();
		void		ExitFontChange();
		
		void		SyncState(ViewLayer *view);	
		void		Play(ViewLayer *view);
		
		void 		Usurp(ServerPicture *newPicture);
		ServerPicture*	StepDown();		
		
		bool		NestPicture(ServerPicture *picture);

		off_t		DataLength() const;
		
		status_t	ImportData(BPrivate::LinkReceiver &link);
		status_t	ExportData(BPrivate::PortLink &link);

private:
friend class	ServerApp;
	
		ServerPicture();
		ServerPicture(const ServerPicture &);
		ServerPicture(const char *fileName, const int32 &offset);
		~ServerPicture();

		int32		fToken;
		BPositionIO	*fData;
		// DrawState	*fState;
		BList		*fPictures;
		ServerPicture	*fUsurped;
};

#endif // __SERVER_PICTURE_H
