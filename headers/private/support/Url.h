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

	status_t	ParseAndSplit();
	status_t	InitCheck() const;

	bool		HasHost() const;
	bool		HasPort() const;
	bool		HasUser() const;
	bool		HasPass() const;
	bool		HasPath() const;

	BString		Proto() const;
	BString		Full() const;
	BString		Host() const;
	BString		Port() const;
	BString		User() const;
	BString		Pass() const;

	BString		proto;
	BString		full;
	BString		host;
	BString		port;
	BString		user;
	BString		pass;
	BString		path;
private:
	status_t	fStatus;
};

}; // namespace Support
}; // namespace BPrivate

#endif // _URL_H

