// TODO: This file is here just to be able to use BDirectWindow
// with BeOS R5. Remove it when we don't need it anymore

#ifndef _APP_SERVER_LINK_H
#define _APP_SERVER_LINK_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <OS.h>
#include "Session.h"

class _BAppServerLink_ {

public:
					_BAppServerLink_();
virtual				~_BAppServerLink_();

		_BSession_	*fSession;
};

#endif
