// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __IppURLConnection_H
#define __IppURLConnection_H

#include "HttpURLConnection.h"

class IppContent;

class IppURLConnection : public HttpURLConnection {
public:
	IppURLConnection(const BUrl &url);
	~IppURLConnection();

	void setIppRequest(IppContent *);
	const IppContent *getIppResponse() const;

	int length();

	ostream &printIppRequest(ostream &);
	ostream &printIppResponse(ostream &);

protected:
	virtual void setRequest();
	virtual void setContent();
	virtual void getContent();

private:
	IppContent *__ippRequest;
	IppContent *__ippResponse;
};

#endif	// __IppURLConnection_H
