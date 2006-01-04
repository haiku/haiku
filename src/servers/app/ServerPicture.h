#ifndef __SERVER_PICTURE_H
#define __SERVER_PICTURE_H

#include <Font.h>
#include <Rect.h>
#include <Region.h>
#include <DataIO.h>
#include <InterfaceDefs.h>

#include <stack>

using std::stack;

class ServerApp;
class ViewLayer;
class ServerPicture {
public:	
		int32		Token() { return fToken; }
		
		void		BeginOp(int16 op);
		void		EndOp();

		void		EnterStateChange();
		void		ExitStateChange();

		void		EnterFontChange();
		void		ExitFontChange();
		
		void		AddInt8(int8 data);
		void		AddInt16(int16 data);
		void		AddInt32(int32 data);
		void		AddInt64(int64 data);
		void		AddFloat(float data);
		void		AddCoord(BPoint data);
		void		AddRect(BRect data);
		void		AddColor(rgb_color data);
		void		AddString(const char *data);
		void		AddData(const void *data, int32 size);
		
		void		SyncState(ViewLayer *view);
		
		void		Play(ViewLayer *view);
		
		const void	*Data() const { return fData.Buffer(); }
		int32		DataLength() const { return fData.BufferLength(); }

private:
friend class	ServerApp;
	
		ServerPicture();
		ServerPicture(const ServerPicture &);
		~ServerPicture();
		
		int32		fToken;
		BMallocIO	fData;
		stack<off_t>	fStack;
		// DrawState	*fState;
};

#endif // __SERVER_PICTURE_H
