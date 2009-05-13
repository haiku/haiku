/*
 * Copyright 2007-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _URL_H
#define _URL_H

#include <String.h>


namespace BPrivate {
namespace Support {

class BUrl : public BString {
public:
								BUrl(const char* url);
								~BUrl();

			status_t			InitCheck() const;

			bool				HasPreferredApplication() const;
			BString				PreferredApplication() const;
			status_t			OpenWithPreferredApplication(
									bool onProblemAskUser = true) const;

			bool				HasHost() const;
			bool				HasPort() const;
			bool				HasUser() const;
			bool				HasPass() const;
			bool				HasPath() const;

	const	BString&			Proto() const;
	const	BString&			Full() const;
	const	BString&			Host() const;
	const	BString&			Port() const;
	const	BString&			User() const;
	const	BString&			Pass() const;
	const	BString&			Path() const;

private:
			status_t			_ParseAndSplit();
			BString				_UrlMimeType() const;

			BString				fProto;
			BString				fFull;
			BString				fHost;
			BString				fPort;
			BString				fUser;
			BString				fPass;
			BString				fPath;
			status_t			fStatus;
};

} // namespace Support
} // namespace BPrivate

#endif // _URL_H

