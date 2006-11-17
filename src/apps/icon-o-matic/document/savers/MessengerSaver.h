/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef MESSENGER_SAVER_H
#define MESSENGER_SAVER_H

#include <Messenger.h>

#include "DocumentSaver.h"

class MessengerSaver : public DocumentSaver {
 public:
								MessengerSaver(const BMessenger& messenger);
	virtual						~MessengerSaver();

	virtual	status_t			Save(Document* document);

 private:
			BMessenger			fMessenger;
};

#endif // MESSENGER_SAVER_H
