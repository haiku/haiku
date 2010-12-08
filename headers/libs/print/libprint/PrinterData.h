/*
 * PrinterData.h
 * Copyright 1999-2000 Y.Takagi All Rights Reserved.
 */

#ifndef __PRINTERDATA_H
#define __PRINTERDATA_H

#include <string>
#include <SerialPort.h>

using namespace std;

class BNode;

class PrinterData {
public:
						PrinterData(BNode* node = NULL);
	virtual				~PrinterData();

	virtual	void		Load();
	virtual	void		Save();

			const string&	GetDriverName() const;
			const string&	GetPrinterName() const;
			const string&	GetComments() const;
			const string&	GetTransport() const;
			int				GetProtocolClass() const;

			void			SetPrinterName(const char* printerName);
			void			SetComments(const char* comments);
			void			SetProtocolClass(int protocolClass);

			bool			GetPath(string& path) const;

protected:
						PrinterData(const PrinterData &printer_data);

			PrinterData&	operator=(const PrinterData &printer_data);

	BNode*	fNode;

private:
	string	fDriverName;
	string	fPrinterName;
	string	fComments;
	string	fTransport;
	int		fProtocolClass;
};


inline const string&
PrinterData::GetDriverName() const
{
	return fDriverName;
}


inline const string&
PrinterData::GetPrinterName() const
{
	return fPrinterName;
}


inline const string&
PrinterData::GetComments() const
{
	return fComments;
}


inline const string&
PrinterData::GetTransport() const
{
	return fTransport;
}


inline int
PrinterData::GetProtocolClass() const
{
	return fProtocolClass;
}


inline void
PrinterData::SetPrinterName(const char* printerName)
{
	fPrinterName = printerName;
}


inline void
PrinterData::SetComments(const char* comments)
{
	fComments = comments;
}


inline void
PrinterData::SetProtocolClass(int protocolClass)
{
	fProtocolClass = protocolClass;
}

#endif	// __PRINTERDATA_H
