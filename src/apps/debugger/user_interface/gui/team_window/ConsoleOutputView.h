/*
 * Copyright 2013-2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONSOLE_OUTPUT_VIEW_H_
#define CONSOLE_OUTPUT_VIEW_H_


#include <GroupView.h>
#include <ObjectList.h>


class BButton;
class BCheckBox;
class BTextView;


class ConsoleOutputView : public BGroupView {
public:
								ConsoleOutputView();
								~ConsoleOutputView();

	static	ConsoleOutputView*	Create();
									// throws

			void				ConsoleOutputReceived(
									int32 fd, const BString& output);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

			void				LoadSettings(const BMessage& settings);
			status_t			SaveSettings(BMessage& settings);

private:
			struct OutputInfo;
			typedef BObjectList<OutputInfo> OutputInfoList;

private:
			void				_Init();
	static	int32				_OutputWorker(void* arg);
			void				_HandleConsoleOutput(OutputInfo* info);

private:
			BCheckBox*			fStdoutEnabled;
			BCheckBox*			fStderrEnabled;
			BTextView*			fConsoleOutput;
			BButton*			fClearButton;
			OutputInfoList*		fPendingOutput;
			sem_id				fWorkToDoSem;
			thread_id			fOutputWorker;
};



#endif	// CONSOLE_OUTPUT_VIEW_H
