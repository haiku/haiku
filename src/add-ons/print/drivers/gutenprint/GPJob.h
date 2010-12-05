/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#ifndef GP_JOB_H
#define GP_JOB_H

#include<gutenprint/gutenprint.h>

#include<list>

#include<String.h>

#include "GPBand.h"
#include "GPJobConfiguration.h"
#include "OutputStream.h"
#include "Rectangle.h"


class GPJob
{
public:
			GPJob();
			~GPJob();

	void	SetApplicationName(const BString& applicationName);
	void	SetConfiguration(GPJobConfiguration* configuration);
	void	SetOutputStream(OutputStream* outputStream);

	status_t	Begin();
	void		End();

	status_t	PrintPage(list<GPBand*>& bands);

	void		GetErrorMessage(BString& message);

private:
	RectInt32	GetPrintRectangle(list<GPBand*>& bands);
	GPBand*		FindBand(int line);
	void		FillRow(GPBand* band, unsigned char* data, size_t size,
		int line);
	void		FillWhite(unsigned char* data, size_t size);

	void				Init();
	void				Reset();
	int					Width();
	int					Height();
	stp_image_status_t	GetRow(unsigned char* data, size_t size, int row);
	const char*			GetAppname();
	void				Conclude();
	void				Write(const char* data, size_t size);
	void				ReportError(const char* data, size_t size);

	static void			ImageInit(stp_image_t* image);
	static void			ImageReset(stp_image_t* image);
	static int			ImageWidth(stp_image_t* image);
	static int			ImageHeight(stp_image_t *image);
	static stp_image_status_t	ImageGetRow(stp_image_t* image,
		unsigned char* data, size_t size, int row);
	static const char*	ImageGetAppname(stp_image_t* image);
	static void			ImageConclude(stp_image_t *image);
	static void			OutputFunction(void *cookie, const char *data,
		size_t size);
	static void			ErrorFunction(void *cookie, const char *data,
		size_t size);

	BString				fApplicationName;
	OutputStream*		fOutputStream;
	GPJobConfiguration*	fConfiguration;

	bool					fHasPages;
	stp_image_t				fImage;
	stp_vars_t*				fVariables;
	const stp_printer_t*	fPrinter;
	RectInt32				fPrintRect;
	list<GPBand*>*			fBands;
	GPBand*					fCachedBand;
	status_t				fStatus;
	BString					fErrorMessage;
};
#endif
