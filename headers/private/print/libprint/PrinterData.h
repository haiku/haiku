/*
 * PrinterData.h
 * Copyright 1999-2000 Y.Takagi All Rights Reserved.
 */

#ifndef __PRINTERDATA_H
#define __PRINTERDATA_H

#include <string>
#include <SerialPort.h>

#if (!__MWERKS__ || defined(MSIPL_USING_NAMESPACE))
using namespace std;
#else 
#define std
#endif

class BNode;

class PrinterData {
public:
	PrinterData(BNode *node = NULL);
	~PrinterData();
	void load();
	void save();

	const string &getDriverName() const;
	const string &getPrinterName() const;
	const string &getComments() const;
	const string &getTransport() const;
	int getProtocolClass() const;

	void  setPrinterName(const char *printer_name);
	void  setComments(const char *comments);
	void  setProtocolClass(int protocolClass);

	bool getPath(string &path) const;

protected:
	PrinterData(const PrinterData &printer_data);
	PrinterData &operator = (const PrinterData &printer_data);

private:
	string fDriverName;
	string fPrinterName;
	string fComments;
	string fTransport;
	int    fProtocolClass;
	
	BNode  *fNode;
};

inline const string &PrinterData::getDriverName() const
{
	return fDriverName;
}

inline const string &PrinterData::getPrinterName() const
{
	return fPrinterName;
}

inline const string &PrinterData::getComments() const
{
	return fComments;
}

inline const string &PrinterData::getTransport() const
{
	return fTransport;
}

inline int PrinterData::getProtocolClass() const
{
	return fProtocolClass;
}

inline void PrinterData::setPrinterName(const char *printer_name)
{
	fPrinterName = printer_name;
}

inline void PrinterData::setComments(const char *comments)
{
	fComments = comments;
}

inline void PrinterData::setProtocolClass(int protocolClass)
{
	fProtocolClass = protocolClass;
}

#endif	// __PRINTERDATA_H
