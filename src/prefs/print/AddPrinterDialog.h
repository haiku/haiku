#ifndef ADDPRINTERDIALOG_H
#define ADDPRINTERDIALOG_H

class AddPrinterDialog;

#include <Window.h>

class AddPrinterDialog : public BWindow
{
	typedef BWindow Inherited;
public:
	static status_t Start();

private:
			AddPrinterDialog();
	void	BuildGUI();
};

#endif
