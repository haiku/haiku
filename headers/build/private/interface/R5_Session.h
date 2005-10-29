// TODO: This file is here just to be able to use BDirectWindow
// with BeOS R5. Remove it when we don't need it anymore

#ifndef _SESSION_H
#define _SESSION_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <Region.h>
#include <Rect.h>
#include <OS.h>

class _BSession_ {

public:
				_BSession_(port_id receivePort, port_id sendPort);
				_BSession_(int32, char *);
virtual			~_BSession_();

		char	*sread();
		void	sread_coo(float *coo);
		void	sread_coo_a(float *coo);
		void	sread_point(BPoint *point);
		void	sread_point_a(BPoint *point);
		void	sread_rect(clipping_rect *rect);
		void	sread_rect(BRect *rect);
		void	sread_rect_a(BRect *rect);
		void	sread(int32 size, void *data);
		void	sread2(int32 size, void *data);
		void	sreadd(int32 size, void *data);

		void	swrite_s(int16 s);
		void	swrite_l(int32 l);
		void	swrite(char *string);
		void	swrite_point(const BPoint *point);
		void	swrite_point_a(const BPoint *point);
		void	swrite_coo(float *coo);
		void	swrite_coo_a(float *coo);
		void	swrite_rect(const clipping_rect *rect);
		void	swrite_rect(const BRect *rect);
		void	swrite_rect_a(const BRect *rect);
		void	swrite(int32 size, void *data);

//		void	get_other(message *);
//		void	add_to_buffer(message *);

		void	recv_buffer();
		void	send_buffer();

		void	sync();
		void	sync_debug();
		void	sclose();
		void	xclose();

private:
		port_id	fSendPort;
		port_id	fReceivePort;

		char	*fSendBuffer;
		int32	fSendBufferSize;
		int32	fSendSize;
		char	*fReceiveBuffer;
		int32	fReceiveSize;
		int32	fReceivePos;
};

extern _IMPEXP_BE _BSession_ *main_session;

#endif
