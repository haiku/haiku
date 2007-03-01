#ifndef __SERVER_PICTURE_H
#define __SERVER_PICTURE_H

#include <DataIO.h>

#include <PictureDataWriter.h>

class ServerApp;
class ViewLayer;
class BPrivate::LinkReceiver;
class ServerPicture : public PictureDataWriter {
public:	
		int32		Token() { return fToken; }
		
		void		EnterStateChange();
		void		ExitStateChange();

		void		EnterFontChange();
		void		ExitFontChange();
		
		void		SyncState(ViewLayer *view);
		
		void		Play(ViewLayer *view);
		
		const void	*Data() const { return fData.Buffer(); }
		int32		DataLength() const { return fData.BufferLength(); }
		
		status_t	ImportData(BPrivate::LinkReceiver &link);

private:
friend class	ServerApp;
	
		ServerPicture();
		ServerPicture(const ServerPicture &);
		~ServerPicture();

		int32		fToken;
		BMallocIO	fData;
		// DrawState	*fState;
};

#endif // __SERVER_PICTURE_H
