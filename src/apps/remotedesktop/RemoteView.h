/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef REMOTE_VIEW_H
#define REMOTE_VIEW_H

#include <Cursor.h>
#include <NetEndpoint.h>
#include <ObjectList.h>
#include <View.h>

class BBitmap;
class NetReceiver;
class NetSender;
class StreamingRingBuffer;

struct engine_state;

class RemoteView : public BView {
public:
									RemoteView(BRect frame, uint16 listenPort);
virtual								~RemoteView();

		status_t					InitCheck();

virtual	void						AttachedToWindow();

virtual	void						Draw(BRect updateRect);

virtual	void						MouseMoved(BPoint where, uint32 code,
										const BMessage *dragMessage);
virtual	void						MouseDown(BPoint where);
virtual	void						MouseUp(BPoint where);

virtual	void						KeyDown(const char *bytes, int32 numBytes);
virtual	void						KeyUp(const char *bytes, int32 numBytes);

virtual	void						MessageReceived(BMessage *message);

private:
		void						_SendMouseMessage(uint16 code,
										BPoint where);
		void						_SendKeyMessage(uint16 code,
										const char *bytes, int32 numBytes);

static	int							_StateCompareByKey(const uint32 *key,
										const engine_state *state);
		void						_CreateState(uint32 token);
		void						_DeleteState(uint32 token);
		engine_state *				_FindState(uint32 token);

static	int32						_DrawEntry(void *data);
		void						_DrawThread();

		BRect						_BuildInvalidateRect(BPoint *points,
										int32 pointCount);

		status_t					fInitStatus;
		bool						fIsConnected;

		StreamingRingBuffer *		fReceiveBuffer;
		StreamingRingBuffer *		fSendBuffer;
		BNetEndpoint *				fReceiveEndpoint;
		BNetEndpoint *				fSendEndpoint;
		NetReceiver *				fReceiver;
		NetSender *					fSender;

		bool						fStopThread;
		thread_id					fDrawThread;

		BBitmap *					fOffscreenBitmap;
		BView *						fOffscreen;

		BCursor						fViewCursor;
		BBitmap *					fCursorBitmap;
		BRect						fCursorFrame;
		bool						fCursorVisible;

		BObjectList<engine_state>	fStates;
};

#endif // REMOTE_VIEW_H
