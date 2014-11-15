/*
 * Copyright 2012-2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef WATCH_PROMPT_WINDOW_H
#define WATCH_PROMPT_WINDOW_H


#include <Window.h>

#include "ExpressionInfo.h"
#include "types/Types.h"


class Architecture;
class BMenuField;
class BTextControl;
class SourceLanguage;
class Watchpoint;
class UserInterfaceListener;


class WatchPromptWindow : public BWindow, private ExpressionInfo::Listener
{
public:
								WatchPromptWindow(Architecture* architecture,
									target_addr_t address, uint32 type,
									int32 length,
									UserInterfaceListener* listener);

								~WatchPromptWindow();

	static	WatchPromptWindow*	Create(Architecture* architecture,
									target_addr_t address, uint32 type,
									int32 length,
									UserInterfaceListener* listener);
									// throws


	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Show();

	// ExpressionInfo::Listener
	virtual	void				ExpressionEvaluated(ExpressionInfo* info,
									status_t result, ExpressionResult* value);

private:
			void	 			_Init();


private:
			target_addr_t		fInitialAddress;
			uint32				fInitialType;
			int32				fInitialLength;
			Architecture*		fArchitecture;
			target_addr_t		fRequestedAddress;
			int32				fRequestedLength;
			BTextControl*		fAddressInput;
			BTextControl*		fLengthInput;
			ExpressionInfo*		fAddressExpressionInfo;
			ExpressionInfo*		fLengthExpressionInfo;
			BMenuField*			fTypeField;
			UserInterfaceListener* fListener;
			BButton*			fWatchButton;
			BButton*			fCancelButton;
			SourceLanguage*		fLanguage;
};

#endif // WATCH_PROMPT_WINDOW_H
