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

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

extern "C" {
	typedef BDataIO *(*PFN_init_transport)(BMessage *);
	typedef void (*PFN_exit_transport)(void);
}

class TransportException {
private:
	string fWhat;
public:
	TransportException(const string &what_arg) : fWhat(what_arg) {}
	const char *what() const { return fWhat.c_str(); }
};

class Transport {
public:
	Transport(const PrinterData *printer_data);
	~Transport();
	void write(const void *buffer, size_t size) throw(TransportException);
	bool check_abort() const;
	const string &last_error() const;

protected:
	void set_last_error(const char *e);
	Transport(const Transport &);
	Transport &operator = (const Transport &);

private:
	image_id           fImage;
	PFN_init_transport fInitTransport;
	PFN_exit_transport fExitTransport;
	BDataIO            *fDataStream;
	bool               fAbort;
	string             fLastErrorString;
};

#endif	// __TRANSPORT_H
