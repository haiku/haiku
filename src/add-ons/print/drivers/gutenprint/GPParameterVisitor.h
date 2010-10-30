/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#ifndef GP_PARAMETER_VISITOR_H
#define GP_PARAMETER_VISITOR_H

#include <gutenprint/gutenprint.h>

#include <Size.h>
#include <Rect.h>


extern const char* kJobMode;
extern const char* kJob;

extern const char* kPrintingMode;
extern const char* kColor;
extern const char* kBlackAndWhite;

extern const char* kResolution;
extern const char* kFakeResolutionKey;

extern const char* kPageSize;

extern const char* kChannelBitDepth;


class GPParameterVisitor
{
public:
					GPParameterVisitor();
	virtual			~GPParameterVisitor();
	
			void	Visit(const stp_printer_t* printer);
			void	VisitParameter(stp_parameter_list_t list,
				const stp_parameter_t* parameter, stp_parameter_t* description);
			void	VisitStringList(stp_parameter_t* parameter);

	virtual	bool	BeginParameter(const char* name, const char* displayName,
						stp_parameter_class_t parameterClass) = 0;
	// key is null if there is no default value
	virtual	void	DefaultStringParameter(const char* name,
						const char* key) = 0;
	virtual	void	StringParameterSize(const char* name, int size) = 0;
	virtual	void	StringParameter(const char* name, const char* key,
						const char* displayName) = 0;
	virtual	void	ResolutionParameter(const char* name, const char* key,
						const char* displayName, int x, int y) = 0;
	virtual	void	PageSizeParameter(const char* name, const char* key,
						const char* displayName, BSize pageSize,
						BRect imageableArea) = 0;
	virtual	void	EndParameter(const char* name) = 0;
	virtual	void	EndVisit() = 0;

private:
			void	AddMissingResolution();

	stp_vars_t*		fVariables;
	bool			fHasResolutionParameter;
};


#endif
