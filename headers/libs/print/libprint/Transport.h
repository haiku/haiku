/*
 * Transport.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __TRANSPORT_H
#define __TRANSPORT_H

#include <image.h>
#include <string>

class BDataIO;
class PrinterData;


using namespace std;


extern "C" {
	typedef BDataIO *(*PFN_init_transport)(BMessage *);
	typedef void (*PFN_exit_transport)(void);
}


class TransportException {
public:
					TransportException(const string &what_arg);
	const char*		What() const;

private:
	string fWhat;
};


class Transport {
public:
					Transport(const PrinterData* printerData);
					~Transport();

	void			Write(const void *buffer, size_t size)
						/* throw (TransportException) */;
	void			Read(void *buffer, size_t size)
						/* throw (TransportException) */;
	bool			CheckAbort() const;
	bool			IsPrintToFileCanceled() const;
	const string&	LastError() const;

protected:
					Transport(const Transport& transport);
					Transport &operator=(const Transport& transport);

	void			SetLastError(const char* message);

private:
	image_id			fImage;
	PFN_init_transport	fInitTransport;
	PFN_exit_transport	fExitTransport;
	BDataIO*			fDataStream;
	bool				fAbort;
	string				fLastErrorString;
};

#endif	// __TRANSPORT_H
