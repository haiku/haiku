/*
 * Copyright 2007-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _URL_H
#define _URL_H

#include <String.h>

namespace BPrivate {
namespace Support {

class BUrl : public BString {
public:
								BUrl(const char *url);
								~BUrl();

			status_t			InitCheck() const;

			bool				HasPreferredApplication() const;
			BString				PreferredApplication() const;
			status_t			OpenWithPreferredApplication(bool onProblemAskUser = true) const;

			bool				HasHost() const;
			bool				HasPort() const;
			bool				HasUser() const;
			bool				HasPass() const;
			bool				HasPath() const;

			BString				Proto() const;
			BString				Full() const;
			BString				Host() const;
			BString				Port() const;
			BString				User() const;
			BString				Pass() const;
			BString				Path() const;

			status_t			UnurlString(BString &string);

private:
			status_t			_ParseAndSplit();
			BString				_UrlMimeType() const;

			BString				proto;
			BString				full;
			BString				host;
			BString				port;
			BString				user;
			BString				pass;
			BString				path;
			status_t			fStatus;
};

}; // namespace Support
}; // namespace BPrivate

#endif // _URL_H

