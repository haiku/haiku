#ifndef _Output_h
#define _Output_h

#include <Window.h>
#include <TextView.h>
#include <ScrollBar.h>
#include <Button.h>
#include <TabView.h>

const uint32 MSG_OUTPUT_RESET		= 'outr';
const uint32 MSG_OUTPUT_RESET_ALL	= 'opra';

class OutputView : public BView
{
public:
	OutputView(BRect frame);
	virtual void	FrameResized(float width, float height);

	void			Add(const char* text)	{m_pTextView->Insert(text);}
	void			Clear()					{m_pTextView->Delete(0,m_pTextView->TextLength());}

private:
	BTextView*		m_pTextView;
	BScrollBar*		m_pScrollBar;
};



class Output : public BWindow
{
public:
	static Output*	Instance();
	~Output();
	virtual bool	QuitRequested();
	virtual void	MessageReceived(BMessage* msg);
	virtual void	FrameMoved(BPoint point);

	void			AddTab(const char* text, int32 index);

	void            Post(const char* text, uint32 index);

private: // functions
	Output();
	void			Add(const char* text, OutputView* view);

private: // data
	static Output*	m_instance;
	BTab*			m_pAllTab;

	OutputView*		m_pAll;

	BButton*		m_pReset;
	BButton*		m_pResetAll;
	
	BTabView* 		fTabView;
	
	BList*			fTabsList;
	BList*			fOutputViewsList;
	BRect			fBounds; // Bounds for tabs
};


#endif // _Output_h
