#ifndef _OBOS_APPLICATION_H_
#define _OBOS_APPLICATION_H_

#include <SupportDefs.h>
#include <Bitmap.h>

class PortLink;

class TestApp
{
public:
	TestApp(void);
	virtual ~TestApp(void);

	// Replaces BLooper message looping
	void MainLoop(void);
	
	// Hmm... What do these do? ;^)
	void DispatchMessage(int32 code, int8 *buffer);
	virtual void DispatchMessage(BMessage *msg, BHandler *handler);
	virtual void MessageReceived(BMessage *msg);
	virtual void ReadyToRun(void);
	bool Run(void);
	void ShowCursor(void);
	void HideCursor(void);
	void ObscureCursor(void);
	bool IsCursorHidden(void);
	void SetCursor(const void *cursor);
	void SetCursor(const BCursor *cursor, bool sync=true);
	void SetCursor(BBitmap *bmp);

	port_id messageport, serverport;
	char *signature;
	bool initialized;
	PortLink *serverlink;

	static int32 TestGraphics(void *data);
	
	// Testing functions:
	void SetLowColor(uint8 r, uint8 g, uint8 b);
	void SetHighColor(uint8 r, uint8 g, uint8 b);

	void TestScreenStates(void);
	void TestArcs(void);
	void TestBeziers(void);
	void TestEllipses(void);
	void TestFonts(void);
	void TestLines(void);
	void TestPolygon(void);
	void TestRects(void);
	void TestRegions(void);
	void TestShape(void);
	void TestTriangle(void);
	void TestCursors(void);
};

#endif